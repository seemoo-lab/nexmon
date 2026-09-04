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

int available_in_txpowers[MAX_TX_POWER];
int available_in_txpowers_count = 0;
int available_out_txpowers[MAX_TX_POWER];
int available_out_txpowers_count = 0;
int osdep_sockfd_in = -1;
int osdep_sockfd_out = -1;

static struct wif *_wi_in, *_wi_out;

struct devices
{
    int fd_in,  arptype_in;
    int fd_out, arptype_out;
    int fd_rtc;
} dev;

int current_channel = 0;
char *osdep_iface_in = NULL;
char *osdep_iface_out = NULL;

int osdep_start(char *interface1, char *interface2)
{
    osdep_iface_in = malloc(strlen(interface1) + 1);
    strcpy(osdep_iface_in, interface1);

	osdep_iface_out = malloc(strlen(interface2) + 1);
    strcpy(osdep_iface_out, interface2);

	/* open the replay interface */
	_wi_out = wi_open(interface2);
	if (!_wi_out){
		printf("open interface %s failed.\n", interface2);
		return 1;
	}

	dev.fd_out = wi_fd(_wi_out);

	if(!strcmp(interface1, interface2)){

		/* open the packet source */
		_wi_in = _wi_out;
		dev.fd_in = dev.fd_out;

		/* XXX */
		dev.arptype_in = dev.arptype_out;
	}
	else{

		/* open the packet source */
		_wi_in = wi_open(interface1);
		if (!_wi_in){
			printf("open interface %s failed.\n", interface1);
			return 1;
		}

		dev.fd_in = wi_fd(_wi_in);
	}

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
	if(_wi_out == _wi_in){
		wi_set_channel(_wi_out, channel);
	}else{
		wi_set_channel(_wi_out, channel);
		wi_set_channel(_wi_in, channel);
	}
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

	if(_wi_out == _wi_in){
		wi_set_rate(_wi_out, rate);
	}else{
		wi_set_rate(_wi_out, rate);
		wi_set_rate(_wi_in, rate);
	}

}

#ifdef __linux__
void osdep_init_txpowers()
{
    //Stupid? Just try rates to find working ones...
    //Anybody know how to get a proper list of supported rates?

    if (!osdep_iface_out) {
      printf("D'oh, open interface %s first, idiot...\n", osdep_iface_out);
      return;
    }

    struct iwreq wreq;
    int old_txpower, i;

    osdep_sockfd_out = socket(AF_INET, SOCK_DGRAM, 0);
    if(osdep_sockfd_out < 0) {
      printf("WTF? Couldn't open socket. Something is VERY wrong...\n");
      return;
    }

    memset(&wreq, 0, sizeof(struct iwreq));
    strncpy(wreq.ifr_name, osdep_iface_out, IFNAMSIZ);
    wreq.u.power.flags = 0;

    if(ioctl(osdep_sockfd_out, SIOCGIWTXPOW, &wreq) < 0) {
      perror("Can't get TX power from card: ");
      return;
    }

    old_txpower = wreq.u.txpower.value;
    printf("Interface %s current TX power: %i dBm\n", osdep_iface_out, wreq.u.txpower.value);

    for (i=0; i<MAX_TX_POWER; i++) {
      wreq.u.txpower.value = i;
      if(ioctl(osdep_sockfd_out, SIOCSIWTXPOW, &wreq) == 0) {
	available_out_txpowers[available_out_txpowers_count] = i;
	available_out_txpowers_count++;
      }
    }

    //Reset to initial value
    wreq.u.txpower.value = old_txpower;
    ioctl(osdep_sockfd_out, SIOCSIWTXPOW, &wreq);

    printf("Interface %s available TX powers: ", osdep_iface_out);
    for (i=0; i<available_out_txpowers_count; i++) {
      printf("%i, ", available_out_txpowers[i]);
    }


	if(strcmp(osdep_iface_in, osdep_iface_out)){
		printf("\n");

		osdep_sockfd_in = socket(AF_INET, SOCK_DGRAM, 0);
		if(osdep_sockfd_in < 0) {
		  printf("WTF? Couldn't open socket. Something is VERY wrong...\n");
		  return;
		}

		memset(&wreq, 0, sizeof(struct iwreq));
		strncpy(wreq.ifr_name, osdep_iface_in, IFNAMSIZ);
		wreq.u.power.flags = 0;

		if(ioctl(osdep_sockfd_in, SIOCGIWTXPOW, &wreq) < 0) {
		  perror("Can't get TX power from card: ");
		  return;
		}

		old_txpower = wreq.u.txpower.value;
		printf("Interface %s current TX power: %i dBm\n", osdep_iface_in, wreq.u.txpower.value);

		for (i=0; i<MAX_TX_POWER; i++) {
		  wreq.u.txpower.value = i;
		  if(ioctl(osdep_sockfd_in, SIOCSIWTXPOW, &wreq) == 0) {
		available_in_txpowers[available_in_txpowers_count] = i;
		available_in_txpowers_count++;
		  }
		}

		//Reset to initial value
		wreq.u.txpower.value = old_txpower;
		ioctl(osdep_sockfd_in, SIOCSIWTXPOW, &wreq);

		printf("Interface %s available TX powers: ", osdep_iface_in);
		for (i=0; i<available_in_txpowers_count; i++) {
		  printf("%i, ", available_in_txpowers[i]);
		}

	}

    printf("\b\b dBm\n");
}

void osdep_random_txpower(int min) {
    long rnd;
    struct iwreq wreq;

    if (! available_out_txpowers_count) {  //This also makes sure the socket exists ;)
      printf("Can't set random TX power since no TX power is known to me :(\n");
      return;
    }

    do {
      rnd = random() % available_out_txpowers_count;
    } while(available_out_txpowers[rnd] < min);

    memset(&wreq, 0, sizeof(struct iwreq));
    strncpy(wreq.ifr_name, osdep_iface_out, IFNAMSIZ);

    ioctl(osdep_sockfd_out, SIOCGIWTXPOW, &wreq);
    wreq.u.txpower.value = available_out_txpowers[rnd];
    ioctl(osdep_sockfd_out, SIOCSIWTXPOW, &wreq);

	if(strcmp(osdep_iface_in, osdep_iface_out)){
		if (! available_in_txpowers_count) {  //This also makes sure the socket exists ;)
		  printf("Can't set random TX power since no TX power is known to me :(\n");
		  return;
		}

		do {
		  rnd = random() % available_in_txpowers_count;
		} while(available_in_txpowers[rnd] < min);

		memset(&wreq, 0, sizeof(struct iwreq));
		strncpy(wreq.ifr_name, osdep_iface_in, IFNAMSIZ);

		ioctl(osdep_sockfd_in, SIOCGIWTXPOW, &wreq);
		wreq.u.txpower.value = available_in_txpowers[rnd];
		ioctl(osdep_sockfd_in, SIOCSIWTXPOW, &wreq);
	}
}

int osdep_get_max_txpower() {
    int max_out = 0, max_in = 0, i;

    if (! available_out_txpowers_count) {
      printf("You forget to osdep_init_txpowers()!\n");
      return 0;
    }

    for (i=0; i<available_out_txpowers_count; i++) {
      if (available_out_txpowers[i] > max_out) max_out = available_out_txpowers[i];
    }

	if(strcmp(osdep_iface_in, osdep_iface_out)){
		if (! available_in_txpowers_count) {
		  printf("You forget to osdep_init_txpowers()!\n");
		  return 0;
		}

		for (i=0; i<available_in_txpowers_count; i++) {
		  if (available_in_txpowers[i] > max_in) max_in = available_in_txpowers[i];
		}
	}

    return max_out > max_in ? max_in : max_out;
}
#endif
