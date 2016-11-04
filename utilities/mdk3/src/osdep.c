#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "osdep/osdep.h"
#include "osdep.h"

#ifdef __linux__
 #include <linux/wireless.h>
 #include <sys/socket.h>
 #include <sys/ioctl.h>
#else
 #warning NOT COMPILING FOR LINUX - Ghosting (IDS Evasion) will not be available
#endif

//Thats the max tx power we try to set, your fault if the hardware dies :P
#define MAX_TX_POWER 50

int available_txpowers[MAX_TX_POWER];
int available_txpowers_count = 0;
int osdep_sockfd = -1;

static struct wif *_wi_in, *_wi_out;

struct devices
{
    int fd_in,  arptype_in;
    int fd_out, arptype_out;
    int fd_rtc;
} dev;

int current_channel = 0;
char *osdep_iface = NULL;

int osdep_start(char *interface)
{
    osdep_iface = malloc(strlen(interface) + 1);
    strcpy(osdep_iface, interface);
    
    /* open the replay interface */
    _wi_out = wi_open(interface);
    if (!_wi_out)
    	return 1;
    dev.fd_out = wi_fd(_wi_out);

    /* open the packet source */
    _wi_in = _wi_out;
    dev.fd_in = dev.fd_out;

    /* XXX */
    dev.arptype_in = dev.arptype_out;
    
    return 0;
}


int osdep_send_packet(struct packet *pkt)
{
	struct wif *wi = _wi_out; /* XXX globals suck */
	if (wi_write(wi, pkt->data, pkt->len, NULL) == -1) {
		switch (errno) {
		case EAGAIN:
		case ENOBUFS:
			usleep(10000);
			return 0; /* XXX not sure I like this... -sorbo */
		}

		perror("wi_write()");
		return -1;
	}

	return 0;
}


struct packet osdep_read_packet()
{
	struct wif *wi = _wi_in; /* XXX */
	int rc;
	struct packet pkt;

	do {
	  rc = wi_read(wi, pkt.data, MAX_PACKET_SIZE, NULL);
	  if (rc == -1) {
	    perror("wi_read()");
	    pkt.len = 0;
	    return pkt;
	  }
	} while (rc < 1);

	pkt.len = rc;
	return pkt;
}


void osdep_set_channel(int channel)
{
    wi_set_channel(_wi_out, channel);
    current_channel = channel;
}


int osdep_get_channel()
{
    return current_channel;
}


void osdep_set_rate(int rate)
{
    int i, valid = 0;
    
    for (i=0; i<VALID_RATE_COUNT; i++) {
      if (VALID_BITRATES[i] == rate) valid = 1;
    }
    
    if (!valid) printf("BUG: osdep_set_rate(): Invalid bitrate selected!\n");
    
    wi_set_rate(_wi_out, rate);
}

#ifdef __linux__
void osdep_init_txpowers()
{
    //Stupid? Just try rates to find working ones...
    //Anybody know how to get a proper list of supported rates?

    if (!osdep_iface) {
      printf("D'oh, open interface first, idiot...\n");
      return;
    }

    struct iwreq wreq;
    int old_txpower, i;
    
    osdep_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(osdep_sockfd < 0) {
      printf("WTF? Couldn't open socket. Something is VERY wrong...\n");
      return;
    }
    
    memset(&wreq, 0, sizeof(struct iwreq));
    strncpy(wreq.ifr_name, osdep_iface, IFNAMSIZ);
    wreq.u.power.flags = 0;
    
    if(ioctl(osdep_sockfd, SIOCGIWTXPOW, &wreq) < 0) {
      perror("Can't get TX power from card: ");
      return;
    }
  
    old_txpower = wreq.u.txpower.value;
    printf("Current TX power: %i dBm\n", wreq.u.txpower.value);
    
    for (i=0; i<MAX_TX_POWER; i++) {
      wreq.u.txpower.value = i;
      if(ioctl(osdep_sockfd, SIOCSIWTXPOW, &wreq) == 0) {
	available_txpowers[available_txpowers_count] = i;
	available_txpowers_count++;
      }
    }
    
    //Reset to initial value
    wreq.u.txpower.value = old_txpower;
    ioctl(osdep_sockfd, SIOCSIWTXPOW, &wreq);
    
    printf("Available TX powers: ");
    for (i=0; i<available_txpowers_count; i++) {
      printf("%i, ", available_txpowers[i]);
    }
    printf("\b\b dBm\n");
}

void osdep_random_txpower(int min) {
    long rnd;
    struct iwreq wreq;
    
    if (! available_txpowers_count) {  //This also makes sure the socket exists ;)
      printf("Can't set random TX power since no TX power is known to me :(\n");
      return;
    }
    
    do {
      rnd = random() % available_txpowers_count;
    } while(available_txpowers[rnd] < min);
    
    memset(&wreq, 0, sizeof(struct iwreq));
    strncpy(wreq.ifr_name, osdep_iface, IFNAMSIZ);
    
    ioctl(osdep_sockfd, SIOCGIWTXPOW, &wreq);
    wreq.u.txpower.value = available_txpowers[rnd];
    ioctl(osdep_sockfd, SIOCSIWTXPOW, &wreq);
}

int osdep_get_max_txpower() {
    int max = 0, i;
    
    if (! available_txpowers_count) {
      printf("You forget to osdep_init_txpowers()!\n");
      return 0;
    }
  
    for (i=0; i<available_txpowers_count; i++) {
      if (available_txpowers[i] > max) max = available_txpowers[i];
    }
    
    return max;
}
#endif
