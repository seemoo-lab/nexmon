/*
 *  802.11 WEP / WPA-PSK Key Cracker
 *
 *  Copyright (C) 2006-2016 Thomas d'Otreppe <tdotreppe@aircrack-ng.org>
 *  Copyright (C) 2004, 2005 Christophe Devine
 *
 *  Advanced WEP attacks developed by KoreK
 *  WPA-PSK  attack code developed by Joshua Wright
 *  SHA1 MMX assembly code written by Simon Marechal
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

#define _GNU_SOURCE

#include <sys/types.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include <ctype.h>
#include <err.h>
#include <math.h>
#include <limits.h>
#include <jni.h>

#include "version.h"
#include "crypto.h"
#include "pcap.h"
#include "uniqueiv.h"
#include "aircrack-ng.h"
#include "sha1-sse2.h"
#include "osdep/byteorder.h"
#include "common.h"
#include "wkp-frame.h"
#include "linecount.h"
#include "wpapsk.h"
#include "myaircrackwep.h"

#ifdef HAVE_SQLITE
#include <sqlite3.h>
sqlite3 *db;
#endif

// libgcrypt thread callback definition for libgcrypt < 1.6.0
#ifdef USE_GCRYPT
	#if GCRYPT_VERSION_NUMBER < 0x010600
		GCRY_THREAD_OPTION_PTHREAD_IMPL;
	#endif
#endif

static unsigned char ZERO[32] =
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00";

/* stats global data */

static int _speed_test;
struct timeval t_begin;			 /* time at start of attack      */
struct timeval t_stats;			 /* time since last update       */
struct timeval t_kprev;			 /* time at start of window      */
struct timeval t_dictup;		/* next dictionary total read   */
long long int nb_kprev;			 /* last  # of keys tried        */
long long int nb_tried;			 /* total # of keys tried        */

/* IPC global data */

unsigned char *buffer = NULL;			/* from read_thread */
struct AP_info *ap_1st;			 /* first item in linked list    */
pthread_mutex_t mx_apl;			 /* lock write access to ap LL   */
pthread_mutex_t mx_eof;			 /* lock write access to nb_eof  */
pthread_mutex_t mx_ivb;			 /* lock access to ivbuf array   */
pthread_mutex_t mx_dic;			 /* lock access to opt.dict      */
pthread_cond_t  cv_eof;			 /* read EOF condition variable  */
int  nb_eof = 0;				 /* # of threads who reached eof */
long nb_pkt = 0;				 /* # of packets read so far     */
int mc_pipe[256][2];			 /* master->child control pipe   */
int cm_pipe[256][2];			 /* child->master results pipe   */
int bf_pipe[256][2];			 /* bruteforcer 'queue' pipe	 */
int bf_nkeys[256];
unsigned char bf_wepkey[64];
int wepkey_crack_success = 0;
int close_aircrack = 0;
int id=0;
pthread_t tid[MAX_THREADS];
struct WPA_data wpa_data[MAX_THREADS];
int wpa_wordlists_done = 0;
static pthread_mutex_t mx_nb = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mx_wpastats = PTHREAD_MUTEX_INITIALIZER;

JNIEnv *cb_jnienv;
jobject cb_obj;
jmethodID cb_mid;
JavaVM* jvm;


int cb_id;
int stop_att = 0;

#define	GOT_IV	0x00000001
#define	USE_IV	0x00000002
#define K01_IV	0x00000010
#define K02_IV	0x00000020
#define K03_IV	0x00000040
#define K04_IV	0x00000080
#define K05_IV	0x00000100
#define K06_IV	0x00000200
#define K07_IV	0x00000400
#define K08_IV	0x00000800
#define K09_IV	0x00001000
#define K10_IV	0x00002000
#define K11_IV	0x00004000
#define K12_IV	0x00008000
#define K13_IV	0x00010000
#define K14_IV	0x00020000
#define K15_IV	0x00040000
#define K16_IV	0x00080000
#define K17_IV	0x00100000

#define UPDATE_ERROR 1
#define UPDATE_SUCCESS 2
#define UPDATE_RUNNING 3

typedef struct
{
	int off1;
	int off2;
	void *buf1;
	void *buf2;
}
read_buf;

read_buf rb, crb;

int K_COEFF[N_ATTACKS] =
{
	15, 13, 12, 12, 12, 5, 5, 5, 3, 4, 3, 4, 3, 13, 4, 4, -20
};

int PTW_DEFAULTWEIGHT[1] = { 256 };
int PTW_DEFAULTBF[PTW_KEYHSBYTES] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const unsigned char R[256] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20
	, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40
	, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60
	, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80
	, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100
	, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116
	, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132
	, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148
	, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164
	, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180
	, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196
	, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212
	, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228
	, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244
	, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

char usage[] =
"\n"
"  %s - (C) 2006-2015 Thomas d\'Otreppe\n"
"  http://www.aircrack-ng.org\n"
"\n"
"  usage: aircrack-ng [options] <.cap / .ivs file(s)>\n"
"\n"
"  Common options:\n"
"\n"
"      -a <amode> : force attack mode (1/WEP, 2/WPA-PSK)\n"
"      -e <essid> : target selection: network identifier\n"
"      -b <bssid> : target selection: access point's MAC\n"
"      -p <nbcpu> : # of CPU to use  (default: all CPUs)\n"
"      -q         : enable quiet mode (no status output)\n"
"      -C <macs>  : merge the given APs to a virtual one\n"
"      -l <file>  : write key to file\n"
"\n"
"  Static WEP cracking options:\n"
"\n"
"      -c         : search alpha-numeric characters only\n"
"      -t         : search binary coded decimal chr only\n"
"      -h         : search the numeric key for Fritz!BOX\n"
"      -d <mask>  : use masking of the key (A1:XX:CF:YY)\n"
"      -m <maddr> : MAC address to filter usable packets\n"
"      -n <nbits> : WEP key length :  64/128/152/256/512\n"
"      -i <index> : WEP key index (1 to 4), default: any\n"
"      -f <fudge> : bruteforce fudge factor,  default: 2\n"
"      -k <korek> : disable one attack method  (1 to 17)\n"
"      -x or -x0  : disable bruteforce for last keybytes\n"
"      -x1        : last keybyte bruteforcing  (default)\n"
"      -x2        : enable last  2 keybytes bruteforcing"
"%s"
"      -y         : experimental  single bruteforce mode\n"
"      -K         : use only old KoreK attacks (pre-PTW)\n"
"      -s         : show the key in ASCII while cracking\n"
"      -M <num>   : specify maximum number of IVs to use\n"
"      -D         : WEP decloak, skips broken keystreams\n"
"      -P <num>   : PTW debug:  1: disable Klein, 2: PTW\n"
"      -1         : run only 1 try to crack key with PTW\n"
"\n"
"  WEP and WPA-PSK cracking options:\n"
"\n"
"      -w <words> : path to wordlist(s) filename(s)\n"
"\n"
"  WPA-PSK options:\n"
"\n"
"      -E <file>  : create EWSA Project file v3\n"
"      -J <file>  : create Hashcat Capture file\n"
"      -S         : WPA cracking speed test\n"
#ifdef HAVE_SQLITE
"      -r <DB>    : path to airolib-ng database\n"
"                   (Cannot be used with -w)\n"
#endif
"\n"
"  Other options:\n"
"\n"
"      -u         : Displays # of CPUs & MMX/SSE support\n"
"      --help     : Displays this usage screen\n"
"\n";


char * progname = NULL;
int intr_read = 0;

int safe_write( int fd, void *buf, size_t len );

char info_buffer[1024];
JNIEnv *cb_jnienv;
jobject cb_obj;
jmethodID cb_mid;
jmethodID cb_mid_stats;

int cb_id;



void info_message(char* mes, int reason) {
    jstring jmes, jid;
    
        
        jmes = (*cb_jnienv)->NewStringUTF(cb_jnienv, mes);
    
        (*cb_jnienv)->CallVoidMethod(cb_jnienv, cb_obj, cb_mid, jmes, (jint) reason, (jint) cb_id);

        (*cb_jnienv)->DeleteLocalRef(cb_jnienv, jmes);
    
}

void info_message_thread(char* mes, int reason, JNIEnv* env) {
	    jstring jmes;
    
              
        jmes = (*env)->NewStringUTF(env, mes);
    
        (*env)->CallVoidMethod(env, cb_obj, cb_mid, jmes, (jint) reason, (jint) cb_id);

        (*env)->DeleteLocalRef(env, jmes);
    
}

void clean_exit(int ret)
{
	struct AP_info *ap_cur;
	struct AP_info *ap_prv;
	struct AP_info *ap_next;
	struct ST_info *st_tmp;
	int i=0;
// 	int j=0, k=0, attack=0;
	int child_pid;

	char tmpbuf[128];
	memset(tmpbuf, 0, 128);

	if(ret && !opt.is_quiet)
	{
		printf("\nQuitting aircrack-ng...\n");
		fflush(stdout);
	}
	close_aircrack = 1;

	for( i = 0; i < opt.nbcpu; i++ )
	{
            #ifdef CYGWIN
            close( mc_pipe[i][1] );
            close( bf_pipe[i][1] );
            #else
            safe_write( mc_pipe[i][1], (void *) "EXIT\r", 5 );
            safe_write( bf_pipe[i][1], (void *) tmpbuf, 64 );
            #endif
        }

	for (i = 0; i < id; i++) {
		if (pthread_join(tid[i], NULL) != 0) {
//	 			printf("Can't join thread %d\n", i);
		}
	}

#ifndef OLD_SSE_CORE
	for (i = 0; i < MAX_THREADS; i++)
		free_ssecore(i);
#endif

	if (opt.totaldicts) {
		for (i = 0; i < opt.totaldicts; i++) {
			if (opt.dicts[i] != NULL) {
				free(opt.dicts[i]);
				opt.dicts[i] = NULL;
			}
		}
	}

	if (rb.buf1 != NULL)
	{
		free(rb.buf1);
		rb.buf1=NULL;
	}

	if (rb.buf2 != NULL)
	{
		free(rb.buf2);
		rb.buf2=NULL;
	}

	if (crb.buf1 != NULL)
	{
		free(crb.buf1);
		crb.buf1=NULL;
	}

	if (crb.buf2 != NULL)
	{
		free(crb.buf2);
		crb.buf2=NULL;
	}

	if (buffer != NULL) {
		free(buffer);
		buffer = NULL;
	}

	if(wep.ivbuf != NULL)
	{
		free(wep.ivbuf);
		wep.ivbuf = NULL;
	}

	ap_prv = NULL;
	ap_cur = ap_1st;

	while( ap_cur != NULL )
	{
		if( ap_cur->ivbuf != NULL )
		{
			free(ap_cur->ivbuf);
			ap_cur->ivbuf = NULL;
		}


		while (ap_cur->st_1st != NULL) {
			st_tmp = ap_cur->st_1st;
			ap_cur->st_1st = ap_cur->st_1st->next;
			free(st_tmp);
			st_tmp = NULL;
		}

		uniqueiv_wipe( ap_cur->uiv_root );
		ap_cur->uiv_root = NULL;

		if( ap_cur->ptw_clean != NULL )
		{
			if( ap_cur->ptw_clean->allsessions != NULL )
			{
				free(ap_cur->ptw_clean->allsessions);
				ap_cur->ptw_clean->allsessions=NULL;
			}
			free(ap_cur->ptw_clean);
			ap_cur->ptw_clean = NULL;
		}

		if( ap_cur->ptw_vague != NULL )
		{
			if( ap_cur->ptw_vague->allsessions != NULL )
			{
				free(ap_cur->ptw_vague->allsessions);
				ap_cur->ptw_vague->allsessions = NULL;
			}
			free(ap_cur->ptw_vague);
			ap_cur->ptw_vague = NULL;
		}

		ap_prv = ap_cur;
		ap_cur = ap_cur->next;
	}

	ap_cur = ap_1st;

	while( ap_cur != NULL )
	{
		ap_next = ap_cur;
		ap_cur = ap_cur->next;
		free(ap_next);
		ap_next = NULL;
	}

	ap_prv = NULL;

// 	attack = A_s5_1;
// 	printf("Please wait for evaluation...\n");
// 	for(i=0; i<(256*256*256); i++)
// 	{
// 		if((all_ivs[i].used & GOT_IV) && !(all_ivs[i].used & USE_IV))
// 			j++;
//
// 		if((all_ivs[i].used & GOT_IV) && (all_ivs[i].used & (1<<(attack+4)) ) )
// 		{
// 			printf("IV %02X:%02X:%02X used for %d\n", (i/(256*256)), ((i&0xFFFF)/(256)), (i&0xFF), attack);
// 			k++;
// 		}
// 	}
//
// 	printf("%d unused IVs\n", j);
// 	printf("%d used IVs for %d\n", k, attack);

	if (progname != NULL) {
		free(progname);
		progname = NULL;
	}

	child_pid=fork();

	if(child_pid==-1)
	{
	  /* do error stuff here */
	}
	if(child_pid!=0)
	{
	  /* The parent process exits here. */

	  //exit(0);
	}

	//_exit(ret);
}

void sighandler( int signum )
{
	#if ((defined(__INTEL_COMPILER) || defined(__ICC)) && defined(DO_PGO_DUMP))
	_PGOPTI_Prof_Dump();
	#endif
#if !defined(__CYGWIN__)
        // We can't call this on cygwin or we will sometimes end up
        // having all our threads die with exit code 35584 fairly reproducable
        // at around 2.5-3% of runs
        signal( signum, sighandler );
#endif

	if( signum == SIGQUIT )
		clean_exit( SUCCESS );
// 		_exit( SUCCESS );

	if( signum == SIGTERM )
		clean_exit( FAILURE );
// 		_exit( FAILURE );

	if( signum == SIGINT )
	{
	#if ((defined(__INTEL_COMPILER) || defined(__ICC)) && defined(DO_PGO_DUMP))
		clean_exit( FAILURE );
//		_exit( FAILURE );
	#else
/*		if(intr_read > 0)*/
			clean_exit( FAILURE );
/*		else
			intr_read++;*/
	#endif
	}

	if( signum == SIGWINCH )
		printf( "\33[2J\n" );
}

void eof_wait( int *eof_notified )
{

	if( *eof_notified == 0 )
	{
		*eof_notified = 1;

		/* tell the master thread we reached EOF */

		pthread_mutex_lock( &mx_eof );
		nb_eof++;
		pthread_cond_broadcast( &cv_eof );
		pthread_mutex_unlock( &mx_eof );
	}
		usleep( 100000 );
}

int wpa_send_passphrase(char *key, struct WPA_data* data, int lock);

inline int wpa_send_passphrase(char *key, struct WPA_data* data, int lock)
{
	int delta = 0, i = 0, fincnt = 0;
	off_t tmpword = 0;

	pthread_mutex_lock(&data->mutex);

	if (!opt.dictfinish) {
		delta = chrono(&t_dictup, 0);

		if ((int)delta >= 2) {
			for (; i < opt.totaldicts; i++) {
				if (opt.dictidx[i].loaded) {
					fincnt++;
					continue;
				}

				if (opt.dictidx[i].dictsize > READBUF_BLKSIZE) {
					tmpword			= (long double)linecount(opt.dicts[i], opt.dictidx[i].dictpos, 32);
					opt.dictidx[i].wordcount+= tmpword;
					opt.wordcount		+= tmpword;
					opt.dictidx[i].dictpos	+= (READBUF_BLKSIZE*32);

					if (opt.dictidx[i].dictpos >= opt.dictidx[i].dictsize)
						opt.dictidx[i].loaded = 1;

					// Only process a chunk then come back later for more.
					break;
				}
			}

			if (fincnt == opt.totaldicts)
				opt.dictfinish	= 1;
			else
				delta		= chrono(&t_dictup, 1);
		}
	}

	if ((data->back+1) % data->nkeys == data->front)
	{
		if (lock != 0)
		{
			// wait until there's room in the queue
			pthread_cond_wait(&data->cond, &data->mutex);
		}
		else
		{
			pthread_mutex_unlock(&data->mutex);
			return 0; // full queue!
		}
	}

	// put one key in the buffer:
	memcpy(data->key_buffer + data->back*128, key, 128);
	data->back = (data->back+1) % data->nkeys;

	pthread_mutex_unlock(&data->mutex);

	return 1;
}

int wpa_receive_passphrase(char *key, struct WPA_data* data);

inline int wpa_receive_passphrase(char *key, struct WPA_data* data)
{
	pthread_mutex_lock(&data->mutex);

	if (data->front==data->back)
	{
		pthread_mutex_unlock(&data->mutex);
		return 0; // empty queue!
	}

	// get one key from the buffer:
	memcpy(key, data->key_buffer + data->front*128, 128);
	data->front = (data->front+1) % data->nkeys;

	// signal that there's now room in the queue for more keys
	pthread_cond_signal(&data->cond);
	pthread_mutex_unlock(&data->mutex);

	return 1;
}

/* Returns number of BSSIDs.

    Return value is negative for failures
*/
int checkbssids(char *bssidlist)
{
	int first = 1;
	int failed = 0;
	int i = 0;
	char *list, *frontlist, *tmp;
	int nbBSSID = 0;

	if(bssidlist == NULL) return -1;

#define IS_X(x) ((x) == 'X' || (x) == 'x')
#define VALID_CHAR(x)   ((IS_X(x)) || hexCharToInt(x) > -1)

#define VALID_SEP(arg)	( ((arg) == '_') || ((arg) == '-') || ((arg) == ':') )
	frontlist = list = strdup(bssidlist);
	do
	{
		tmp = strsep(&list, ",");

		if (tmp == NULL)
			break;

		++nbBSSID;

		if(strlen(tmp) != 17) failed = 1;

		//first byte
		if(!VALID_CHAR(tmp[ 0])) failed = 1;
		if(!VALID_CHAR(tmp[ 1])) failed = 1;
		if(!VALID_SEP( tmp[ 2])) failed = 1;

		//second byte
		if(!VALID_CHAR(tmp[ 3])) failed = 1;
		if(!VALID_CHAR(tmp[ 4])) failed = 1;
		if(!VALID_SEP( tmp[ 5])) failed = 1;

		//third byte
		if(!VALID_CHAR(tmp[ 6])) failed = 1;
		if(!VALID_CHAR(tmp[ 7])) failed = 1;
		if(!VALID_SEP( tmp[ 8])) failed = 1;

		//fourth byte
		if(!VALID_CHAR(tmp[ 9])) failed = 1;
		if(!VALID_CHAR(tmp[10])) failed = 1;
		if(!VALID_SEP( tmp[11])) failed = 1;

		//fifth byte
		if(!VALID_CHAR(tmp[12])) failed = 1;
		if(!VALID_CHAR(tmp[13])) failed = 1;
		if(!VALID_SEP( tmp[14])) failed = 1;

		//sixth byte
		if(!VALID_CHAR(tmp[15])) failed = 1;
		if(!VALID_CHAR(tmp[16])) failed = 1;

		if(failed) {
			free(frontlist);
			return -1;
		}

		if(first)
		{
			for(i=0; i< 17; i++) {
				if( IS_X(tmp[i])) {
					free(frontlist);
					return -1;
				}
			}

			opt.firstbssid = (unsigned char *) malloc(sizeof(unsigned char));
			if (opt.firstbssid == NULL){
				free(frontlist);
				return -1;
			}
			getmac(tmp, 1, opt.firstbssid);
			first = 0;
		}

	} while(list);

	// Success
	free(frontlist);
	return nbBSSID;
}

int mergebssids(char * bssidlist, unsigned char * bssid)
{
	struct mergeBSSID * list_prev;
	struct mergeBSSID * list_cur;
	char * mac = NULL;
	char * list = NULL;
	char * tmp = NULL;
	char * tmp2 = NULL;
	int next, i, found;

	// Do not convert if equal to first bssid
	if (memcmp(opt.firstbssid, bssid, 6) == 0)
		return 1;

	list_prev = NULL;
	list_cur = opt.bssid_list_1st;

	while (list_cur != NULL)
	{
		if (memcmp(list_cur->bssid, bssid, 6) == 0)
		{
			if (list_cur->convert)
				memcpy(bssid, opt.firstbssid, 6);

			return list_cur->convert;
		}

		list_prev = list_cur;
		list_cur = list_cur->next;
	}

	// Not found, check if it has to be converted
	mac = (char *) malloc(18);

	if (!mac)
	{
		perror( "malloc failed" );
		return -1;
	}

	snprintf(mac, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
		bssid[0], bssid[1], bssid[2],
		bssid[3], bssid[4], bssid[5]);
	mac[17] = 0;

	tmp2 = list = strdup(bssidlist);

	// skip first element (because it doesn't have to be converted
	// It already has the good value
	tmp = strsep(&list, ",");

	next = found = 0;

	do
	{
		next=0;
		tmp = strsep(&list, ",");
		if (tmp == NULL)
			break;

		// Length already checked, no need to check it again

		for( i = 0; i < 17; ++i)
		{
			if((IS_X(tmp[i]) || VALID_SEP(tmp[i]))) continue;

			if(toupper((int)tmp[i]) != (int)mac[i])
			{
				// Not found
				next = 1;
				break;
			}
		}

		if(next == 0)
		{
			found = 1;
			break;
		}
	}
	while (list);

	// Free memory
	if(mac != NULL)
		free(mac);
	if(tmp2 != NULL)
		free(tmp2);

	// Add the result to the list
	list_cur = (struct mergeBSSID *) malloc(sizeof(struct mergeBSSID));

	if (!list_cur)
	{
			perror( "malloc failed" );
			return -1;
	}

	list_cur->convert = found;
	list_cur->next = NULL;
	memcpy(list_cur->bssid, bssid, 6);

	if (opt.bssid_list_1st == NULL)
		opt.bssid_list_1st = list_cur;
	else
		list_prev->next = list_cur;

	// Do not forget to convert if it was successful
	if (list_cur->convert)
		memcpy(bssid, opt.firstbssid, 6);

#undef VALID_CHAR
#undef VALID_SEP
#undef IS_X

	return list_cur->convert;
}

/* fread isn't atomic, sadly */

int atomic_read( read_buf *rb, int fd, int len, void *buf )
{
	int n;

	if( close_aircrack )
		return( CLOSE_IT );

	if( rb->buf1 == NULL )
	{
		rb->buf1 = malloc( 65536 );
		rb->buf2 = malloc( 65536 );

		if( rb->buf1 == NULL || rb->buf2 == NULL )
			return( 0 );

		rb->off1 = 0;
		rb->off2 = 0;
	}

	if( len > 65536 - rb->off1 )
	{
		rb->off2 -= rb->off1;

		memcpy( rb->buf2, rb->buf1 + rb->off1, rb->off2 );
		memcpy( rb->buf1, rb->buf2, rb->off2 );

		rb->off1 = 0;
	}

	if( rb->off2 - rb->off1 >= len )
	{
		memcpy( buf, rb->buf1 + rb->off1, len );
		rb->off1 += len;
		return( 1 );
	}
	else
	{
		n = read( fd, rb->buf1 + rb->off2, 65536 - rb->off2 );

		if( n <= 0 )
			return( 0 );

		rb->off2 += n;

		if( rb->off2 - rb->off1 >= len )
		{
			memcpy( buf, rb->buf1 + rb->off1, len );
			rb->off1 += len;
			return( 1 );
		}
	}

	return( 0 );
}

void read_thread( void *arg )
{
	int fd, n, fmt;
	unsigned z;
	int eof_notified = 0;
// 	int ret=0;

	unsigned char bssid[6];
	unsigned char dest[6];
	unsigned char stmac[6];
	unsigned char *h80211;
	unsigned char *p;
	int weight[16];

	struct ivs2_pkthdr ivs2;
	struct ivs2_filehdr fivs2;
	struct pcap_pkthdr pkh;
	struct pcap_file_header pfh;
	struct AP_info *ap_prv, *ap_cur;
	struct ST_info *st_prv, *st_cur;

	signal( SIGINT, sighandler);

	memset( &rb, 0, sizeof( rb ) );
	ap_cur = NULL;

	memset(&pfh, 0, sizeof(struct pcap_file_header));

	if( ( buffer = (unsigned char *) malloc( 65536 ) ) == NULL )
	{
		/* there is no buffer */

		perror( "malloc failed" );
		goto read_fail;
	}

	h80211 = buffer;

	if( ! opt.is_quiet )
		printf( "Opening %s\n", (char *) arg );

	if( strcmp( arg, "-" ) == 0 )
		fd = 0;
	else
	{
		if( ( fd = open( (char *) arg, O_RDONLY | O_BINARY ) ) < 0 )
		{
			perror( "open failed" );
			goto read_fail;
		}
	}

	if( ! atomic_read( &rb, fd, 4, &pfh ) )
	{
		perror( "read(file header) failed" );
		goto read_fail;
	}

	fmt = FORMAT_IVS;

	if( memcmp( &pfh, IVSONLY_MAGIC, 4 ) != 0 &&
            memcmp( &pfh, IVS2_MAGIC, 4 ) != 0)
	{
		fmt = FORMAT_CAP;

		if( pfh.magic != TCPDUMP_MAGIC &&
			pfh.magic != TCPDUMP_CIGAM )
		{
			fprintf( stderr, "Unsupported file format "
				"(not a pcap or IVs file).\n" );
			goto read_fail;
		}

		/* read the rest of the pcap file header */

		if( ! atomic_read( &rb, fd, 20, (unsigned char *) &pfh + 4 ) )
		{
			perror( "read(file header) failed" );
			goto read_fail;
		}

		/* take care of endian issues and check the link type */

		if( pfh.magic == TCPDUMP_CIGAM )
			SWAP32( pfh.linktype );

		if( pfh.linktype != LINKTYPE_IEEE802_11 &&
			pfh.linktype != LINKTYPE_PRISM_HEADER &&
			pfh.linktype != LINKTYPE_RADIOTAP_HDR &&
			pfh.linktype != LINKTYPE_PPI_HDR)
		{
			fprintf( stderr, "This file is not a regular "
				"802.11 (wireless) capture.\n" );
			goto read_fail;
		}
	}
	else
	{
		if( opt.wep_decloak )
		{
			errx(1, "Can't use decloak wep mode with ivs\n"); /* XXX */
		}

		if (memcmp( &pfh, IVS2_MAGIC, 4 ) == 0)
		{
			fmt = FORMAT_IVS2;

			if( ! atomic_read( &rb, fd, sizeof(struct ivs2_filehdr), (unsigned char *) &fivs2 ) )
			{
				perror( "read(file header) failed" );
				goto read_fail;
			}
			if(fivs2.version > IVS2_VERSION)
			{
				printf( "Error, wrong %s version: %d. Supported up to version %d.\n", IVS2_EXTENSION, fivs2.version, IVS2_VERSION );
				goto read_fail;
			}
		} else if (opt.do_ptw)
			errx(1, "Can't do PTW with old IVS files, recapture without --ivs or use airodump-ng >= 1.0\n"); /* XXX */
	}
	/* avoid blocking on reading the file */

	if( fcntl( fd, F_SETFL, O_NONBLOCK ) < 0 )
	{
		perror( "fcntl(O_NONBLOCK) failed" );
		goto read_fail;
	}

	while( 1 )
	{
		if( close_aircrack )
			break;

		if( fmt == FORMAT_IVS )
		{
			/* read one IV */

			while( ! atomic_read( &rb, fd, 1, buffer ) )
				eof_wait( &eof_notified );

			if( close_aircrack )
				break;

			if( buffer[0] != 0xFF )
			{
				/* new access point MAC */

				bssid[0] = buffer[0];

				while( ! atomic_read( &rb, fd, 5, bssid + 1 ) ) {
					eof_wait( &eof_notified );
					if( close_aircrack )
						goto read_fail;
				}
				
			}

			while( ! atomic_read( &rb, fd, 5, buffer ) ) {
				eof_wait( &eof_notified );
				if( close_aircrack )
						goto read_fail;
			}
		
		}
		else if( fmt == FORMAT_IVS2 )
		{
			while( ! atomic_read( &rb, fd, sizeof( struct ivs2_pkthdr ), &ivs2 ) ) {
				eof_wait( &eof_notified );
				if( close_aircrack )
						goto read_fail;
			}
			

			if(ivs2.flags & IVS2_BSSID)
			{
				while( ! atomic_read( &rb, fd, 6, bssid ) ) {
					eof_wait( &eof_notified );
					if( close_aircrack )
						goto read_fail;
				}
				
				ivs2.len -= 6;
			}

			while( ! atomic_read( &rb, fd, ivs2.len, buffer ) ) {
				eof_wait( &eof_notified );
				if( close_aircrack )
						goto read_fail;
			}
		
		}
		else
		{
			while( ! atomic_read( &rb, fd, sizeof( pkh ), &pkh ) ) {
				eof_wait( &eof_notified );
				if( close_aircrack )
						goto read_fail;
			}
			

			if( pfh.magic == TCPDUMP_CIGAM ) {
				SWAP32( pkh.caplen );
				SWAP32( pkh.len );
			}

			if( pkh.caplen <= 0 || pkh.caplen > 65535 )
			{
				fprintf( stderr, "\nInvalid packet capture length %d - "
					"corrupted file?\n", pkh.caplen );
				eof_wait( &eof_notified );
				if( close_aircrack )
						goto read_fail;
				//_exit( FAILURE );
			}

			while( ! atomic_read( &rb, fd, pkh.caplen, buffer ) ) {
				eof_wait( &eof_notified );
				if( close_aircrack )
						goto read_fail;
			}
			

			h80211 = buffer;

			if( pfh.linktype == LINKTYPE_PRISM_HEADER )
			{
				/* remove the prism header */

				if( h80211[7] == 0x40 )
					n = 64;
				else
				{
					n = *(int *)( h80211 + 4 );

					if( pfh.magic == TCPDUMP_CIGAM )
						SWAP32( n );
				}

				if( n < 8 || n >= (int) pkh.caplen )
					continue;

				h80211 += n; pkh.caplen -= n;
			}

			if( pfh.linktype == LINKTYPE_RADIOTAP_HDR )
			{
				/* remove the radiotap header */

				n = *(unsigned short *)( h80211 + 2 );

				if( n <= 0 || n >= (int) pkh.caplen )
					continue;

				h80211 += n; pkh.caplen -= n;
			}

			if( pfh.linktype == LINKTYPE_PPI_HDR )
			{
				/* Remove the PPI header */

				n = le16_to_cpu(*(unsigned short *)( h80211 + 2));

				if( n <= 0 || n>= (int) pkh.caplen )
					continue;

				/* for a while Kismet logged broken PPI headers */
				if ( n == 24 && le16_to_cpu(*(unsigned short *)(h80211 + 8)) == 2 )
					n = 32;

				if( n <= 0 || n>= (int) pkh.caplen )
					continue;

				h80211 += n; pkh.caplen -= n;
			}
		}

		/* prevent concurrent access on the linked list */

		pthread_mutex_lock( &mx_apl );

		nb_pkt++;
		if( close_aircrack )
			goto read_fail;
		if( fmt == FORMAT_CAP )
		{
			/* skip packets smaller than a 802.11 header */

			if( pkh.caplen < 24 )
				goto unlock_mx_apl;

			/* skip (uninteresting) control frames */

			if( ( h80211[0] & 0x0C ) == 0x04 )
				goto unlock_mx_apl;

			/* locate the access point's MAC address */

			switch( h80211[1] & 3 )
			{
				case  0: memcpy( bssid, h80211 + 16, 6 ); break;  //Adhoc
				case  1: memcpy( bssid, h80211 +  4, 6 ); break;  //ToDS
				case  2: memcpy( bssid, h80211 + 10, 6 ); break;  //FromDS
				case  3: memcpy( bssid, h80211 + 10, 6 ); break;  //WDS -> Transmitter taken as BSSID
			}

			switch( h80211[1] & 3 )
			{
				case  0: memcpy( dest, h80211 +  4, 6 ); break;  //Adhoc
				case  1: memcpy( dest, h80211 + 16, 6 ); break;  //ToDS
				case  2: memcpy( dest, h80211 +  4, 6 ); break;  //FromDS
				case  3: memcpy( dest, h80211 + 16, 6 ); break;  //WDS -> Transmitter taken as BSSID
			}

			//skip corrupted keystreams in wep decloak mode
			if(opt.wep_decloak)
			{
				if(dest[0] == 0x01)
					goto unlock_mx_apl;
			}
		}

		if(opt.bssidmerge)
			mergebssids(opt.bssidmerge, bssid);

		if( memcmp( bssid, BROADCAST, 6 ) == 0 )
			/* probe request or such - skip the packet */
			goto unlock_mx_apl;

		if( memcmp( bssid, opt.bssid, 6 ) != 0 )
			goto unlock_mx_apl;

		if( memcmp( opt.maddr, ZERO,      6 ) != 0 &&
			memcmp( opt.maddr, BROADCAST, 6 ) != 0 )
		{
			/* apply the MAC filter */

			if( memcmp( opt.maddr, h80211 +  4, 6 ) != 0 &&
				memcmp( opt.maddr, h80211 + 10, 6 ) != 0 &&
				memcmp( opt.maddr, h80211 + 16, 6 ) != 0 )
				goto unlock_mx_apl;
		}

		/* search the linked list */

		ap_prv = NULL;
		ap_cur = ap_1st;

		while( ap_cur != NULL )
		{
			if( ! memcmp( ap_cur->bssid, bssid, 6 ) )
				break;

			ap_prv = ap_cur;
			ap_cur = ap_cur->next;
		}

		/* if it's a new access point, add it */

		if( ap_cur == NULL )
		{
			if( ! ( ap_cur = (struct AP_info *) malloc(
				sizeof( struct AP_info ) ) ) )
			{
				perror( "malloc failed" );
				break;
			}

			memset( ap_cur, 0, sizeof( struct AP_info ) );

			if( ap_1st == NULL )
				ap_1st = ap_cur;
			else
				ap_prv->next = ap_cur;

			memcpy( ap_cur->bssid, bssid, 6 );

			ap_cur->crypt = -1;

			// Shortcut to set encryption:
			// - WEP is 2 for 'crypt' and 1 for 'amode'.
			// - WPA is 3 for 'crypt' and 2 for 'amode'.
			if (opt.forced_amode)
				ap_cur->crypt = opt.amode + 1;

			if (opt.do_ptw == 1)
			{
				ap_cur->ptw_clean = PTW_newattackstate();
				if (!ap_cur->ptw_clean) {
					perror("PTW_newattackstate()");
					free(ap_cur);
					ap_cur = NULL;
					break;
				}
				ap_cur->ptw_vague = PTW_newattackstate();
				if (!ap_cur->ptw_vague) {
					perror("PTW_newattackstate()");
					free(ap_cur);
					ap_cur = NULL;
					break;
				}
			}
		}

		if( fmt == FORMAT_IVS )
		{
			ap_cur->crypt = 2;

			add_wep_iv:
			/* check for uniqueness first */
			if( close_aircrack )
						goto read_fail;
			if( ap_cur->nb_ivs == 0 )
				ap_cur->uiv_root = uniqueiv_init();

			if( uniqueiv_check( ap_cur->uiv_root, buffer ) == 0 )
			{
				/* add the IV & first two encrypted bytes */

				n = ap_cur->nb_ivs * 5;

				if( n + 5 > ap_cur->ivbuf_size )
				{
					/* enlarge the IVs buffer */

					ap_cur->ivbuf_size += 131072;
					ap_cur->ivbuf = (unsigned char *) realloc(
						ap_cur->ivbuf, ap_cur->ivbuf_size );

					if( ap_cur->ivbuf == NULL )
					{
						perror( "realloc failed" );
						break;
					}
				}

				memcpy( ap_cur->ivbuf + n, buffer, 5 );
				uniqueiv_mark( ap_cur->uiv_root, buffer );
				ap_cur->nb_ivs++;
			}

			goto unlock_mx_apl;
		}
		if( close_aircrack )
						goto read_fail;
		if( fmt == FORMAT_IVS2 )
		{
			if(ivs2.flags & IVS2_ESSID)
			{
				memcpy( ap_cur->essid, buffer, ivs2.len);
			}
			else if(ivs2.flags & IVS2_XOR)
			{
				ap_cur->crypt = 2;

				if (opt.do_ptw) {
					int clearsize;

					clearsize = ivs2.len;

					if (clearsize < opt.keylen+3)
						goto unlock_mx_apl;

					if (PTW_addsession(ap_cur->ptw_clean, buffer, buffer+4, PTW_DEFAULTWEIGHT, 1))
						ap_cur->nb_ivs_clean++;

					if (PTW_addsession(ap_cur->ptw_vague, buffer, buffer+4, PTW_DEFAULTWEIGHT, 1))
						ap_cur->nb_ivs_vague++;

					goto unlock_mx_apl;
				}

				buffer[3] = buffer[4];
				buffer[4] = buffer[5];
				buffer[3] ^= 0xAA;
				buffer[4] ^= 0xAA;
				/* check for uniqueness first */

				if( ap_cur->nb_ivs == 0 )
					ap_cur->uiv_root = uniqueiv_init();

				if( uniqueiv_check( ap_cur->uiv_root, buffer ) == 0 )
				{
					/* add the IV & first two encrypted bytes */

					n = ap_cur->nb_ivs * 5;

					if( n + 5 > ap_cur->ivbuf_size )
					{
						/* enlarge the IVs buffer */

						ap_cur->ivbuf_size += 131072;
						ap_cur->ivbuf = (unsigned char *) realloc(
							ap_cur->ivbuf, ap_cur->ivbuf_size );

						if( ap_cur->ivbuf == NULL )
						{
							perror( "realloc failed" );
							break;
						}
					}


					memcpy( ap_cur->ivbuf + n, buffer, 5 );
					uniqueiv_mark( ap_cur->uiv_root, buffer );
					ap_cur->nb_ivs++;
// 					all_ivs[256*256*buffer[0] + 256*buffer[1] + buffer[2]].used |= GOT_IV;
				}
			}
			else if(ivs2.flags & IVS2_PTW)
			{
				if( close_aircrack )
						goto read_fail;
				ap_cur->crypt = 2;

				if (opt.do_ptw) {
					int clearsize;

					clearsize = ivs2.len;

					if (buffer[5] < opt.keylen)
						goto unlock_mx_apl;
					if( clearsize < (6 + buffer[4]*32 + 16*(signed)sizeof(int)) )
						goto unlock_mx_apl;

					memcpy(weight, buffer+clearsize-15*sizeof(int), 16*sizeof(int));
// 					printf("weight 1: %d, weight 2: %d\n", weight[0], weight[1]);

					if (PTW_addsession(ap_cur->ptw_vague, buffer, buffer+6, weight, buffer[4]))
						ap_cur->nb_ivs_vague++;

					goto unlock_mx_apl;
				}

				buffer[3] = buffer[6];
				buffer[4] = buffer[7];
				buffer[3] ^= 0xAA;
				buffer[4] ^= 0xAA;
				/* check for uniqueness first */

				if( ap_cur->nb_ivs == 0 )
					ap_cur->uiv_root = uniqueiv_init();

				if( uniqueiv_check( ap_cur->uiv_root, buffer ) == 0 )
				{
					/* add the IV & first two encrypted bytes */

					n = ap_cur->nb_ivs * 5;

					if( n + 5 > ap_cur->ivbuf_size )
					{
						/* enlarge the IVs buffer */

						ap_cur->ivbuf_size += 131072;
						ap_cur->ivbuf = (unsigned char *) realloc(
							ap_cur->ivbuf, ap_cur->ivbuf_size );

						if( ap_cur->ivbuf == NULL )
						{
							perror( "realloc failed" );
							break;
						}
					}


					memcpy( ap_cur->ivbuf + n, buffer, 5 );
					uniqueiv_mark( ap_cur->uiv_root, buffer );
					ap_cur->nb_ivs++;
				}
			}
			else if(ivs2.flags & IVS2_WPA)
			{
				ap_cur->crypt = 3;
				memcpy( &ap_cur->wpa, buffer,
					sizeof( struct WPA_hdsk ) );
			}
			if( close_aircrack )
						goto read_fail;
			goto unlock_mx_apl;
		}

		/* locate the station MAC in the 802.11 header */

		st_cur = NULL;

		switch( h80211[1] & 3 )
		{
			case  0: memcpy( stmac, h80211 + 10, 6 ); break;
			case  1: memcpy( stmac, h80211 + 10, 6 ); break;
			case  2:

				/* reject broadcast MACs */

				if( (h80211[4]%2) != 0 ) goto skip_station;
				memcpy( stmac, h80211 +  4, 6 ); break;

			default: goto skip_station; break;
		}

		st_prv = NULL;
		st_cur = ap_cur->st_1st;

		while( st_cur != NULL )
		{
			if( ! memcmp( st_cur->stmac, stmac, 6 ) )
				break;
			if(stop_att)
				goto read_fail;
			st_prv = st_cur;
			st_cur = st_cur->next;
		}

		/* if it's a new supplicant, add it */

		if( st_cur == NULL )
		{
			if( ! ( st_cur = (struct ST_info *) malloc(
				sizeof( struct ST_info ) ) ) )
			{
				perror( "malloc failed" );
				break;
			}

			memset( st_cur, 0, sizeof( struct ST_info ) );

			if( ap_cur->st_1st == NULL )
				ap_cur->st_1st = st_cur;
			else
				st_prv->next = st_cur;

			memcpy( st_cur->stmac, stmac, 6 );
		}

		skip_station:

		/* packet parsing: Beacon or Probe Response */

		if( h80211[0] == 0x80 ||
			h80211[0] == 0x50 )
		{
			if( ap_cur->crypt < 0 )
				ap_cur->crypt = ( h80211[34] & 0x10 ) >> 4;

			p = h80211 + 36;

			while( p < h80211 + pkh.caplen )
			{
				if( p + 2 + p[1] > h80211 + pkh.caplen )
					break;
				if(stop_att)
					goto read_fail;
				if( p[0] == 0x00 && p[1] > 0 && p[2] != '\0' )
				{
					/* found a non-cloaked ESSID */

					n = ( p[1] > 32 ) ? 32 : p[1];

					memset( ap_cur->essid, 0, 33 );
					memcpy( ap_cur->essid, p + 2, n );
				}

				p += 2 + p[1];
			}
		}

		/* packet parsing: Association Request */

		if( h80211[0] == 0x00 )
		{
			p = h80211 + 28;

			while( p < h80211 + pkh.caplen )
			{
				if( p + 2 + p[1] > h80211 + pkh.caplen )
					break;

				if( p[0] == 0x00 && p[1] > 0 && p[2] != '\0' )
				{
					n = ( p[1] > 32 ) ? 32 : p[1];

					memset( ap_cur->essid, 0, 33 );
					memcpy( ap_cur->essid, p + 2, n );
				}

				p += 2 + p[1];
			}
		}

		/* packet parsing: Association Response */

		if( h80211[0] == 0x10 )
		{
			/* reset the WPA handshake state */

			if( st_cur != NULL )
				st_cur->wpa.state = 0;
		}

		/* check if data */

		if( ( h80211[0] & 0x0C ) != 0x08 )
			goto unlock_mx_apl;

		/* check minimum size */

		z = ( ( h80211[1] & 3 ) != 3 ) ? 24 : 30;
		if ( ( h80211[0] & 0x80 ) == 0x80 )
			z+=2; /* 802.11e QoS */

		if( z + 16 > pkh.caplen )
			goto unlock_mx_apl;

		/* check the SNAP header to see if data is encrypted */

		if( h80211[z] != h80211[z + 1] || h80211[z + 2] != 0x03 )
		{
			if( !opt.forced_amode ) {
				ap_cur->crypt = 2;	 /* encryption = WEP */

				/* check the extended IV flag */
				if( ( h80211[z + 3] & 0x20 ) != 0)
				{
					/* encryption = WPA */
					ap_cur->crypt = 3;
				}
			}

			/* check the WEP key index */
				
			if( close_aircrack )
						goto read_fail;
			if( opt.index != 0 &&
				( h80211[z + 3] >> 6 ) != opt.index - 1 )
				goto unlock_mx_apl;

			if (opt.do_ptw) {
				unsigned char *body = h80211 + z;
				int dlen = pkh.caplen - (body-h80211) - 4 -4;
				unsigned char clear[2048];
				int clearsize, i, j, k;
                                int weight[16];

                                if((h80211[1] & 0x03) == 0x03) //30byte header
                                {
                                    body += 6;
                                    dlen -=6;
                                }

				memset(weight, 0, sizeof(weight));
				memset(clear, 0, sizeof(clear));

				/* calculate keystream */
				k = known_clear(clear, &clearsize, weight, h80211, dlen);
				if( close_aircrack )
						goto read_fail;
				if (clearsize < (opt.keylen+3))
					goto unlock_mx_apl;

                                for (j=0; j<k; j++)
                                {
                                    for (i = 0; i < clearsize; i++)
                                            clear[i+(32*j)] ^= body[4+i];
                                }

                                if(k==1)
                                {
                                    if (PTW_addsession(ap_cur->ptw_clean, body, clear, weight, k))
                                            ap_cur->nb_ivs_clean++;
                                        if( close_aircrack )
											goto read_fail;
                                }

                                if (PTW_addsession(ap_cur->ptw_vague, body, clear, weight, k))
                                        ap_cur->nb_ivs_vague++;

                if( close_aircrack )
						goto read_fail;
				goto unlock_mx_apl;
			}

			/* save the IV & first two output bytes */

			memcpy( buffer    , h80211 + z    , 3 );
			memcpy( buffer + 3, h80211 + z + 4, 2 );

            /* Special handling for spanning-tree packets */
            if ( memcmp( h80211 +  4, SPANTREE, 6 ) == 0 ||
                memcmp( h80211 + 16, SPANTREE, 6 ) == 0 )
            {
                buffer[3] = (buffer[3] ^ 0x42) ^ 0xAA;
                buffer[4] = (buffer[4] ^ 0x42) ^ 0xAA;
            }
			if( close_aircrack )
				goto read_fail;
			goto add_wep_iv;
		}

		if( ap_cur->crypt < 0 )
			ap_cur->crypt = 0;	 /* no encryption */

		/* if ethertype == IPv4, find the LAN address */

		z += 6;

		if( z + 20 < pkh.caplen )
		{
			if( h80211[z] == 0x08 && h80211[z + 1] == 0x00 &&
				( h80211[1] & 3 ) == 0x01 )
				memcpy( ap_cur->lanip, &h80211[z + 14], 4 );

			if( h80211[z] == 0x08 && h80211[z + 1] == 0x06 )
				memcpy( ap_cur->lanip, &h80211[z + 16], 4 );
		}

		/* check ethertype == EAPOL */

		if( h80211[z] != 0x88 || h80211[z + 1] != 0x8E )
			goto unlock_mx_apl;

		z += 2;

		ap_cur->eapol = 1;

		/* type == 3 (key), desc. == 254 (WPA) or 2 (RSN) */

		if( h80211[z + 1] != 0x03 ||
			( h80211[z + 4] != 0xFE && h80211[z + 4] != 0x02 ) )
			goto unlock_mx_apl;

		ap_cur->eapol = 0;
		if( !opt.forced_amode )
		ap_cur->crypt = 3;		 /* set WPA */
			if( close_aircrack )
						goto read_fail;
		if( st_cur == NULL )
		{
			pthread_mutex_unlock( &mx_apl );
			continue;
		}

		/* frame 1: Pairwise == 1, Install == 0, Ack == 1, MIC == 0 */

		if( ( h80211[z + 6] & 0x08 ) != 0 &&
			( h80211[z + 6] & 0x40 ) == 0 &&
			( h80211[z + 6] & 0x80 ) != 0 &&
			( h80211[z + 5] & 0x01 ) == 0 )
		{
			memcpy( st_cur->wpa.anonce, &h80211[z + 17], 32 );

			/* authenticator nonce set */
			st_cur->wpa.state = 1;
		}

		/* frame 2 or 4: Pairwise == 1, Install == 0, Ack == 0, MIC == 1 */

		if( ( h80211[z + 6] & 0x08 ) != 0 &&
			( h80211[z + 6] & 0x40 ) == 0 &&
			( h80211[z + 6] & 0x80 ) == 0 &&
			( h80211[z + 5] & 0x01 ) != 0 )
		{
			if( memcmp( &h80211[z + 17], ZERO, 32 ) != 0 )
			{
				memcpy( st_cur->wpa.snonce, &h80211[z + 17], 32 );

								 /* supplicant nonce set */
				st_cur->wpa.state |= 2;
			}

			if( (st_cur->wpa.state & 4) != 4 )
			{
				/* copy the MIC & hsPackets frame */

				st_cur->wpa.eapol_size = ( h80211[z + 2] << 8 )
					+   h80211[z + 3] + 4;

				if (st_cur->wpa.eapol_size == 0 || st_cur->wpa.eapol_size > sizeof(st_cur->wpa.eapol)
					|| pkh.len - z < st_cur->wpa.eapol_size)
				{
					// Ignore the packet trying to crash us.
					st_cur->wpa.eapol_size = 0;
					goto unlock_mx_apl;
				}

				memcpy( st_cur->wpa.keymic, &h80211[z + 81], 16 );
				memcpy( st_cur->wpa.eapol,  &h80211[z], st_cur->wpa.eapol_size );
				memset( st_cur->wpa.eapol + 81, 0, 16 );

									/* hsPackets frame & keymic set */
				st_cur->wpa.state |= 4;

				/* copy the key descriptor version */

				st_cur->wpa.keyver = h80211[z + 6] & 7;
			}
		}

		/* frame 3: Pairwise == 1, Install == 1, Ack == 1, MIC == 1 */

		if( ( h80211[z + 6] & 0x08 ) != 0 &&
			( h80211[z + 6] & 0x40 ) != 0 &&
			( h80211[z + 6] & 0x80 ) != 0 &&
			( h80211[z + 5] & 0x01 ) != 0 )
		{
			if( memcmp( &h80211[z + 17], ZERO, 32 ) != 0 )
			{
				memcpy( st_cur->wpa.anonce, &h80211[z + 17], 32 );

								 /* authenticator nonce set */
				st_cur->wpa.state |= 1;
			}

			if( (st_cur->wpa.state & 4) != 4 )
			{
				/* copy the MIC & hsPackets frame */

				st_cur->wpa.eapol_size = ( h80211[z + 2] << 8 )
					+   h80211[z + 3] + 4;

				if (st_cur->wpa.eapol_size == 0 || st_cur->wpa.eapol_size > sizeof(st_cur->wpa.eapol)
					|| pkh.len - z < st_cur->wpa.eapol_size)
				{
					// Ignore the packet trying to crash us.
					st_cur->wpa.eapol_size = 0;
					goto unlock_mx_apl;
				}

				memcpy( st_cur->wpa.keymic, &h80211[z + 81], 16 );
				memcpy( st_cur->wpa.eapol,  &h80211[z], st_cur->wpa.eapol_size );
				memset( st_cur->wpa.eapol + 81, 0, 16 );

									/* hsPackets frame & keymic set */
				st_cur->wpa.state |= 4;

				/* copy the key descriptor version */

				st_cur->wpa.keyver = h80211[z + 6] & 7;
			}
		}

		if( st_cur->wpa.state == 7 )
		{
			/* got one valid handshake */

			memcpy( st_cur->wpa.stmac, stmac, 6 );
			memcpy( &ap_cur->wpa, &st_cur->wpa,
				sizeof( struct WPA_hdsk ) );
		}

		unlock_mx_apl:
		if( close_aircrack )
						goto read_fail;
		pthread_mutex_unlock( &mx_apl );

		if( ap_cur != NULL )
		{
			if( ( ap_cur->nb_ivs >= opt.max_ivs) ||
			    ( ap_cur->nb_ivs_clean >= opt.max_ivs ) ||
			    ( ap_cur->nb_ivs_vague >= opt.max_ivs ) )
			{
				if( close_aircrack )
						goto read_fail;
				eof_wait( &eof_notified );
				free(buffer);
				return;
			}
		}
	}

	read_fail:

	if(rb.buf1 != NULL)
	{
		free(rb.buf1);
		rb.buf1=NULL;
	}
	if(rb.buf2 != NULL)
	{
		free(rb.buf2);
		rb.buf2=NULL;
	}
	if(buffer != NULL)
	{
		free(buffer);
		buffer=NULL;
	}

	if(close_aircrack)
		return;

	//everything is going down
	//kill( 0, SIGTERM );
	//_exit( FAILURE );
}

void check_thread( void *arg )
{
	int fd, n, fmt;
	unsigned z;
// 	int ret=0;

	unsigned char bssid[6];
	unsigned char dest[6];
	unsigned char stmac[6];
	unsigned char *buffer;
	unsigned char *h80211;
	unsigned char *p;
	int weight[16];

	struct ivs2_pkthdr ivs2;
	struct ivs2_filehdr fivs2;
	struct pcap_pkthdr pkh;
	struct pcap_file_header pfh;
	struct AP_info *ap_prv, *ap_cur;
	struct ST_info *st_prv, *st_cur;

	memset( &crb, 0, sizeof( crb ) );
	ap_cur = NULL;

	if( ( buffer = (unsigned char *) malloc( 65536 ) ) == NULL )
	{
		/* there is no buffer */

		perror( "malloc failed" );
		goto read_fail;
	}

	h80211 = buffer;

	if( ! opt.is_quiet )
		printf( "Opening %s\n", (char *) arg );

	if( strcmp( arg, "-" ) == 0 )
		fd = 0;
	else
	{
		if( ( fd = open( (char *) arg, O_RDONLY | O_BINARY ) ) < 0 )
		{
			perror( "open failed" );
			goto read_fail;
		}
	}

	if( ! atomic_read( &crb, fd, 4, &pfh ) )
	{
		perror( "read(file header) failed" );
		goto read_fail;
	}

	fmt = FORMAT_IVS;

	if( memcmp( &pfh, IVSONLY_MAGIC, 4 ) != 0 &&
            memcmp( &pfh, IVS2_MAGIC, 4 ) != 0)
	{
		fmt = FORMAT_CAP;

		if( pfh.magic != TCPDUMP_MAGIC &&
			pfh.magic != TCPDUMP_CIGAM )
		{
			fprintf( stderr, "Unsupported file format "
				"(not a pcap or IVs file).\n" );
			goto read_fail;
		}

		/* read the rest of the pcap file header */

		if( ! atomic_read( &crb, fd, 20, (unsigned char *) &pfh + 4 ) )
		{
			perror( "read(file header) failed" );
			goto read_fail;
		}

		/* take care of endian issues and check the link type */

		if( pfh.magic == TCPDUMP_CIGAM )
			SWAP32( pfh.linktype );

		if( pfh.linktype != LINKTYPE_IEEE802_11 &&
			pfh.linktype != LINKTYPE_PRISM_HEADER &&
			pfh.linktype != LINKTYPE_RADIOTAP_HDR &&
			pfh.linktype != LINKTYPE_PPI_HDR )
		{
			fprintf( stderr, "This file is not a regular "
				"802.11 (wireless) capture.\n" );
			goto read_fail;
		}
	} else
	{
		if( opt.wep_decloak )
		{
			errx(1, "Can't use decloak wep mode with ivs\n"); /* XXX */
		}
		if (memcmp( &pfh, IVS2_MAGIC, 4 ) == 0)
		{
			fmt = FORMAT_IVS2;

			if( ! atomic_read( &crb, fd, sizeof(struct ivs2_filehdr), (unsigned char *) &fivs2 ) )
			{
				perror( "read(file header) failed" );
				goto read_fail;
			}
			if(fivs2.version > IVS2_VERSION)
			{
				printf( "Error, wrong %s version: %d. Supported up to version %d.\n", IVS2_EXTENSION, fivs2.version, IVS2_VERSION );
				goto read_fail;
			}
		} else if (opt.do_ptw)
			errx(1, "Can't do PTW with old IVS files, recapture without --ivs or use airodump-ng >= 1.0\n"); /* XXX */
	}
	/* avoid blocking on reading the file */

	if( fcntl( fd, F_SETFL, O_NONBLOCK ) < 0 )
	{
		perror( "fcntl(O_NONBLOCK) failed" );
		goto read_fail;
	}

	while( 1 )
	{
		if(close_aircrack)
			break;

		if( fmt == FORMAT_IVS )
		{
			/* read one IV */

			while( ! atomic_read( &crb, fd, 1, buffer ) )
				goto read_fail;

			if( buffer[0] != 0xFF )
			{
				/* new access point MAC */

				bssid[0] = buffer[0];

				while( ! atomic_read( &crb, fd, 5, bssid + 1 ) )
					goto read_fail;
			}

			while( ! atomic_read( &crb, fd, 5, buffer ) )
				goto read_fail;
		}
		else if( fmt == FORMAT_IVS2 )
		{
			while( ! atomic_read( &crb, fd, sizeof( struct ivs2_pkthdr ), &ivs2 ) )
				goto read_fail;

			if(ivs2.flags & IVS2_BSSID)
			{
				while( ! atomic_read( &crb, fd, 6, bssid ) )
					goto read_fail;
				ivs2.len -= 6;
			}

			while( ! atomic_read( &crb, fd, ivs2.len, buffer ) )
				goto read_fail;
		}
		else
		{
			while( ! atomic_read( &crb, fd, sizeof( pkh ), &pkh ) )
				goto read_fail;

			if( pfh.magic == TCPDUMP_CIGAM ) {
				SWAP32( pkh.caplen );
				SWAP32( pkh.len );
			}

			if( pkh.caplen <= 0 || pkh.caplen > 65535 )
			{
				fprintf( stderr, "\nInvalid packet capture length %d - "
					"corrupted file?\n", pkh.caplen );
				goto read_fail;
			}

			while( ! atomic_read( &crb, fd, pkh.caplen, buffer ) )
				goto read_fail;

			h80211 = buffer;

			if( pfh.linktype == LINKTYPE_PRISM_HEADER )
			{
				/* remove the prism header */

				if( h80211[7] == 0x40 )
					n = 64;
				else
				{
					n = *(int *)( h80211 + 4 );

					if( pfh.magic == TCPDUMP_CIGAM )
						SWAP32( n );
				}

				if( n < 8 || n >= (int) pkh.caplen )
					continue;

				h80211 += n; pkh.caplen -= n;
			}

			if( pfh.linktype == LINKTYPE_RADIOTAP_HDR )
			{
				/* remove the radiotap header */

				n = *(unsigned short *)( h80211 + 2 );

				if( n <= 0 || n >= (int) pkh.caplen )
					continue;

				h80211 += n; pkh.caplen -= n;
			}

			if( pfh.linktype == LINKTYPE_PPI_HDR )
			{
				/* Remove the PPI header */

				n = le16_to_cpu(*(unsigned short *)( h80211 + 2));

				if( n <= 0 || n>= (int) pkh.caplen )
					continue;

				/* for a whole Kismet logged broken PPI headers */
				if ( n == 24 && le16_to_cpu(*(unsigned short *)(h80211 + 8)) == 2 )
					n = 32;

				if( n <= 0 || n>= (int) pkh.caplen )
					continue;

				h80211 += n; pkh.caplen -= n;
			}
		}

		/* prevent concurrent access on the linked list */

		pthread_mutex_lock( &mx_apl );

		nb_pkt++;

		if( fmt == FORMAT_CAP )
		{
			/* skip packets smaller than a 802.11 header */

			if( pkh.caplen < 24 )
				goto unlock_mx_apl;

			/* skip (uninteresting) control frames */

			if( ( h80211[0] & 0x0C ) == 0x04 )
				goto unlock_mx_apl;

			/* locate the access point's MAC address */

			switch( h80211[1] & 3 )
			{
				case  0: memcpy( bssid, h80211 + 16, 6 ); break;  //Adhoc
				case  1: memcpy( bssid, h80211 +  4, 6 ); break;  //ToDS
				case  2: memcpy( bssid, h80211 + 10, 6 ); break;  //FromDS
				case  3: memcpy( bssid, h80211 + 10, 6 ); break;  //WDS -> Transmitter taken as BSSID
			}

			switch( h80211[1] & 3 )
			{
				case  0: memcpy( dest, h80211 +  4, 6 ); break;  //Adhoc
				case  1: memcpy( dest, h80211 + 16, 6 ); break;  //ToDS
				case  2: memcpy( dest, h80211 +  4, 6 ); break;  //FromDS
				case  3: memcpy( dest, h80211 + 16, 6 ); break;  //WDS -> Transmitter taken as BSSID
			}

			//skip corrupted keystreams in wep decloak mode
			if(opt.wep_decloak)
			{
				if(dest[0] == 0x01)
					goto unlock_mx_apl;
			}
		}

		if(opt.bssidmerge)
			mergebssids(opt.bssidmerge, bssid);

		if( memcmp( bssid, BROADCAST, 6 ) == 0 )
			/* probe request or such - skip the packet */
			goto unlock_mx_apl;

		if( memcmp( opt.maddr, ZERO,      6 ) != 0 &&
			memcmp( opt.maddr, BROADCAST, 6 ) != 0 )
		{
			/* apply the MAC filter */

			if( memcmp( opt.maddr, h80211 +  4, 6 ) != 0 &&
				memcmp( opt.maddr, h80211 + 10, 6 ) != 0 &&
				memcmp( opt.maddr, h80211 + 16, 6 ) != 0 )
				goto unlock_mx_apl;
		}

		/* search the linked list */

		ap_prv = NULL;
		ap_cur = ap_1st;

		while( ap_cur != NULL )
		{
			if( ! memcmp( ap_cur->bssid, bssid, 6 ) )
				break;

			ap_prv = ap_cur;
			ap_cur = ap_cur->next;
		}

		/* if it's a new access point, add it */

		if( ap_cur == NULL )
		{
			if( ! ( ap_cur = (struct AP_info *) malloc(
				sizeof( struct AP_info ) ) ) )
			{
				perror( "malloc failed" );
				pthread_mutex_unlock( &mx_apl );
				break;
			}

			memset( ap_cur, 0, sizeof( struct AP_info ) );

			if( ap_1st == NULL )
				ap_1st = ap_cur;
			else
				ap_prv->next = ap_cur;

			memcpy( ap_cur->bssid, bssid, 6 );

			ap_cur->crypt = -1;

			// Shortcut to set encryption:
			// - WEP is 2 for 'crypt' and 1 for 'amode'.
			// - WPA is 3 for 'crypt' and 2 for 'amode'.
			if (opt.forced_amode)
				ap_cur->crypt = opt.amode + 1;
		}

		if( fmt == FORMAT_IVS )
		{
			ap_cur->crypt = 2;

			add_wep_iv:
			/* check for uniqueness first */

			if( ap_cur->nb_ivs == 0 )
				ap_cur->uiv_root = uniqueiv_init();

			if( uniqueiv_check( ap_cur->uiv_root, buffer ) == 0 )
			{
				uniqueiv_mark( ap_cur->uiv_root, buffer );
				ap_cur->nb_ivs++;
			}

			goto unlock_mx_apl;
		}

		if( fmt == FORMAT_IVS2 )
		{
			if(ivs2.flags & IVS2_ESSID)
			{
				if (ivs2.len > 32) { // Max length of the ESSID (and length -1 of that field)
					fprintf(stderr, "Invalid SSID length, it must be <= 32\n");
					exit(1);
				}
				memcpy( ap_cur->essid, buffer, ivs2.len);
				if(opt.essid_set && ! strcmp( opt.essid, ap_cur->essid ) )
					memcpy( opt.bssid, ap_cur->bssid, 6 );
			}
			else if(ivs2.flags & IVS2_XOR)
			{
				ap_cur->crypt = 2;

				if (opt.do_ptw) {
					int clearsize;

					clearsize = ivs2.len;

					if (clearsize < opt.keylen+3)
						goto unlock_mx_apl;
				}

				if( ap_cur->nb_ivs == 0 )
					ap_cur->uiv_root = uniqueiv_init();

				if( uniqueiv_check( ap_cur->uiv_root, buffer ) == 0 )
				{
					uniqueiv_mark( ap_cur->uiv_root, buffer );
					ap_cur->nb_ivs++;
				}
			}
			else if(ivs2.flags & IVS2_PTW)
			{
				ap_cur->crypt = 2;

				if (opt.do_ptw) {
					int clearsize;

					clearsize = ivs2.len;

					if (buffer[5] < opt.keylen)
						goto unlock_mx_apl;
					if( clearsize < (6 + buffer[4]*32 + 16*(signed)sizeof(int)) )
						goto unlock_mx_apl;
				}

				if( ap_cur->nb_ivs == 0 )
					ap_cur->uiv_root = uniqueiv_init();

				if( uniqueiv_check( ap_cur->uiv_root, buffer ) == 0 )
				{
					uniqueiv_mark( ap_cur->uiv_root, buffer );
					ap_cur->nb_ivs++;
				}
			}
			else if(ivs2.flags & IVS2_WPA)
			{
				ap_cur->crypt = 3;
				memcpy( &ap_cur->wpa, buffer,
					sizeof( struct WPA_hdsk ) );
			}
			goto unlock_mx_apl;
		}

		/* locate the station MAC in the 802.11 header */

		st_cur = NULL;

		switch( h80211[1] & 3 )
		{
			case  0: memcpy( stmac, h80211 + 10, 6 ); break;
			case  1: memcpy( stmac, h80211 + 10, 6 ); break;
			case  2:

				/* reject broadcast MACs */

				if( (h80211[4]%2) != 0 ) goto skip_station;
				memcpy( stmac, h80211 +  4, 6 ); break;

			default: goto skip_station; break;
		}

		st_prv = NULL;
		st_cur = ap_cur->st_1st;

		while( st_cur != NULL )
		{
			if( ! memcmp( st_cur->stmac, stmac, 6 ) )
				break;

			st_prv = st_cur;
			st_cur = st_cur->next;
		}

		/* if it's a new supplicant, add it */

		if( st_cur == NULL )
		{
			if( ! ( st_cur = (struct ST_info *) malloc(
				sizeof( struct ST_info ) ) ) )
			{
				perror( "malloc failed" );
				pthread_mutex_unlock( &mx_apl );
				break;
			}

			memset( st_cur, 0, sizeof( struct ST_info ) );

			if( ap_cur->st_1st == NULL )
				ap_cur->st_1st = st_cur;
			else
				st_prv->next = st_cur;

			memcpy( st_cur->stmac, stmac, 6 );
		}

		skip_station:

		/* packet parsing: Beacon or Probe Response */

		if( h80211[0] == 0x80 ||
			h80211[0] == 0x50 )
		{
			if( ap_cur->crypt < 0 )
				ap_cur->crypt = ( h80211[34] & 0x10 ) >> 4;

			p = h80211 + 36;

			while( p < h80211 + pkh.caplen )
			{
				if( p + 2 + p[1] > h80211 + pkh.caplen )
					break;

				if( p[0] == 0x00 && p[1] > 0 && p[2] != '\0' )
				{
					/* found a non-cloaked ESSID */

					n = ( p[1] > 32 ) ? 32 : p[1];

					memset( ap_cur->essid, 0, 33 );
					memcpy( ap_cur->essid, p + 2, n );
					if(opt.essid_set && ! strcmp( opt.essid, ap_cur->essid ) )
						memcpy( opt.bssid, ap_cur->bssid, 6 );
				}

				p += 2 + p[1];
			}
		}

		/* packet parsing: Association Request */

		if( h80211[0] == 0x00 )
		{
			p = h80211 + 28;

			while( p < h80211 + pkh.caplen )
			{
				if( p + 2 + p[1] > h80211 + pkh.caplen )
					break;

				if( p[0] == 0x00 && p[1] > 0 && p[2] != '\0' )
				{
					n = ( p[1] > 32 ) ? 32 : p[1];

					memset( ap_cur->essid, 0, 33 );
					memcpy( ap_cur->essid, p + 2, n );
					if(opt.essid_set && ! strcmp( opt.essid, ap_cur->essid ) )
						memcpy( opt.bssid, ap_cur->bssid, 6 );
				}

				p += 2 + p[1];
			}

			/* reset the WPA handshake state */

			if( st_cur != NULL )
				st_cur->wpa.state = 0;
		}

		/* packet parsing: Association Response */

		if( h80211[0] == 0x10 )
		{
			/* reset the WPA handshake state */

			if( st_cur != NULL )
				st_cur->wpa.state = 0;
		}

		/* check if data */

		if( ( h80211[0] & 0x0C ) != 0x08 )
			goto unlock_mx_apl;

		/* check minimum size */

		z = ( ( h80211[1] & 3 ) != 3 ) ? 24 : 30;
		if ( ( h80211[0] & 0x80 ) == 0x80 )
			z+=2; /* 802.11e QoS */

		if( z + 16 > pkh.caplen )
			goto unlock_mx_apl;

		/* check the SNAP header to see if data is encrypted */

		if( h80211[z] != h80211[z + 1] || h80211[z + 2] != 0x03 )
		{
			if( !opt.forced_amode )
			ap_cur->crypt = 2;	 /* encryption = WEP */

			/* check the extended IV flag */

			if( ( h80211[z + 3] & 0x20 ) != 0 && !opt.forced_amode)
			{
				/* encryption = WPA */
				ap_cur->crypt = 3;
			}

			/* check the WEP key index */

			if( opt.index != 0 &&
				( h80211[z + 3] >> 6 ) != opt.index - 1 )
				goto unlock_mx_apl;

			if (opt.do_ptw) {
				unsigned char *body = h80211 + z;
				int dlen = pkh.caplen - (body-h80211) - 4 -4;
				unsigned char clear[2048];
				int clearsize, k;

                                if((h80211[1] & 0x03) == 0x03) //30byte header
                                {
                                    body += 6;
                                    dlen -=6;
                                }

				/* calculate keystream */
				k = known_clear(clear, &clearsize, weight, h80211, dlen);
				if (clearsize < (opt.keylen+3))
					goto unlock_mx_apl;
			}

			/* save the IV & first two output bytes */

			memcpy( buffer    , h80211 + z    , 3 );
			goto add_wep_iv;
		}

		if( ap_cur->crypt < 0 )
			ap_cur->crypt = 0;	 /* no encryption */

		/* if ethertype == IPv4, find the LAN address */

		z += 6;

		if( z + 20 < pkh.caplen )
		{
			if( h80211[z] == 0x08 && h80211[z + 1] == 0x00 &&
				( h80211[1] & 3 ) == 0x01 )
				memcpy( ap_cur->lanip, &h80211[z + 14], 4 );

			if( h80211[z] == 0x08 && h80211[z + 1] == 0x06 )
				memcpy( ap_cur->lanip, &h80211[z + 16], 4 );
		}

		/* check ethertype == EAPOL */

		if( h80211[z] != 0x88 || h80211[z + 1] != 0x8E )
			goto unlock_mx_apl;

		z += 2;

		ap_cur->eapol = 1;

		/* type == 3 (key), desc. == 254 (WPA) or 2 (RSN) */

		if( h80211[z + 1] != 0x03 ||
			( h80211[z + 4] != 0xFE && h80211[z + 4] != 0x02 ) )
			goto unlock_mx_apl;

		ap_cur->eapol = 0;
		ap_cur->crypt = 3;		 /* set WPA */

		if( st_cur == NULL )
		{
			pthread_mutex_unlock( &mx_apl );
			continue;
		}

		/* frame 1: Pairwise == 1, Install == 0, Ack == 1, MIC == 0 */

		if( ( h80211[z + 6] & 0x08 ) != 0 &&
			( h80211[z + 6] & 0x40 ) == 0 &&
			( h80211[z + 6] & 0x80 ) != 0 &&
			( h80211[z + 5] & 0x01 ) == 0 )
		{
			memcpy( st_cur->wpa.anonce, &h80211[z + 17], 32 );

			/* authenticator nonce set */
			st_cur->wpa.state = 1;
		}

		/* frame 2 or 4: Pairwise == 1, Install == 0, Ack == 0, MIC == 1 */

		if( ( h80211[z + 6] & 0x08 ) != 0 &&
			( h80211[z + 6] & 0x40 ) == 0 &&
			( h80211[z + 6] & 0x80 ) == 0 &&
			( h80211[z + 5] & 0x01 ) != 0 )
		{
			if( memcmp( &h80211[z + 17], ZERO, 32 ) != 0 )
			{
				memcpy( st_cur->wpa.snonce, &h80211[z + 17], 32 );

								 /* supplicant nonce set */
				st_cur->wpa.state |= 2;
			}

			if( (st_cur->wpa.state & 4) != 4 )
			{
				/* copy the MIC & hsPackets frame */

				st_cur->wpa.eapol_size = ( h80211[z + 2] << 8 )
					+   h80211[z + 3] + 4;

				if (st_cur->wpa.eapol_size == 0 || st_cur->wpa.eapol_size > sizeof(st_cur->wpa.eapol)
					|| pkh.len - z < st_cur->wpa.eapol_size)
				{
					// Ignore the packet trying to crash us.
					st_cur->wpa.eapol_size = 0;
					goto unlock_mx_apl;
				}

				memcpy( st_cur->wpa.keymic, &h80211[z + 81], 16 );
				memcpy( st_cur->wpa.eapol,  &h80211[z], st_cur->wpa.eapol_size );
				memset( st_cur->wpa.eapol + 81, 0, 16 );

									/* hsPackets frame & keymic set */
				st_cur->wpa.state |= 4;

				/* copy the key descriptor version */

				st_cur->wpa.keyver = h80211[z + 6] & 7;
			}
		}

		/* frame 3: Pairwise == 1, Install == 1, Ack == 1, MIC == 1 */

		if( ( h80211[z + 6] & 0x08 ) != 0 &&
			( h80211[z + 6] & 0x40 ) != 0 &&
			( h80211[z + 6] & 0x80 ) != 0 &&
			( h80211[z + 5] & 0x01 ) != 0 )
		{
			if( memcmp( &h80211[z + 17], ZERO, 32 ) != 0 )
			{
				memcpy( st_cur->wpa.anonce, &h80211[z + 17], 32 );

								 /* authenticator nonce set */
				st_cur->wpa.state |= 1;
			}

			if( (st_cur->wpa.state & 4) != 4 )
			{
				/* copy the MIC & hsPackets frame */

				st_cur->wpa.eapol_size = ( h80211[z + 2] << 8 )
					+   h80211[z + 3] + 4;

				if (st_cur->wpa.eapol_size == 0 || st_cur->wpa.eapol_size > sizeof(st_cur->wpa.eapol)
					|| pkh.len - z < st_cur->wpa.eapol_size)
				{
					// Ignore the packet trying to crash us.
					st_cur->wpa.eapol_size = 0;
					goto unlock_mx_apl;
				}

				memcpy( st_cur->wpa.keymic, &h80211[z + 81], 16 );
				memcpy( st_cur->wpa.eapol,  &h80211[z], st_cur->wpa.eapol_size );
				memset( st_cur->wpa.eapol + 81, 0, 16 );

									/* hsPackets frame & keymic set */
				st_cur->wpa.state |= 4;

				/* copy the key descriptor version */

				st_cur->wpa.keyver = h80211[z + 6] & 7;
			}
		}

		if( st_cur->wpa.state == 7 )
		{
			/* got one valid handshake */

			memcpy( st_cur->wpa.stmac, stmac, 6 );
			memcpy( &ap_cur->wpa, &st_cur->wpa,
				sizeof( struct WPA_hdsk ) );
		}

		unlock_mx_apl:

		pthread_mutex_unlock( &mx_apl );

		if( ap_cur != NULL )
			if( ap_cur->nb_ivs >= opt.max_ivs )
				break;

	}

	read_fail:

	if(crb.buf1 != NULL)
	{
		free(crb.buf1);
		crb.buf1 = NULL;
	}
	if(crb.buf2 != NULL)
	{
		free(crb.buf2);
		crb.buf2 = NULL;
	}
	if(buffer != NULL)
	{
		free(buffer);
		buffer = NULL;
	}

	return;
}

/* timing routine */

float chrono( struct timeval *start, int reset )
{
	float delta;
	struct timeval current;

	gettimeofday( &current, NULL );

	delta = ( current.tv_sec  - start->tv_sec  ) + (float)
		( current.tv_usec - start->tv_usec ) / 1000000;

	if( reset )
		gettimeofday( start, NULL );

	return( delta );
}

/* signal-safe I/O routines */

int safe_read( int fd, void *buf, size_t len )
{
	int n;
	size_t sum = 0;
	char  *off = (char *) buf;

	while( sum < len )
	{
		if( ! ( n = read( fd, (void *) off, len - sum ) ) )
                {
			return( 0 );
                }
		if( n < 0 && errno == EINTR ) continue;
		if( n < 0 ) return( n );

		sum += n;
		off += n;
	}

	return( sum );
}

int safe_write( int fd, void *buf, size_t len )
{
	int n;
	size_t sum = 0;
	char  *off = (char *) buf;

	while( sum < len )
	{
		if( ( n = write( fd, (void *) off, len - sum ) ) < 0 )
		{
			if( errno == EINTR ) continue;
			return( n );
		}

		sum += n;
		off += n;
	}

	return( sum );
}

/* each thread computes the votes over a subset of the IVs */

int crack_wep_thread( void *arg )
{
	long xv, min, max;
	unsigned char jj[256];
	unsigned char S[256], Si[256];
	unsigned char K[64];

	unsigned char io1, o1, io2, o2;
	unsigned char Sq, dq, Kq, jq, q;
	unsigned char S1, S2, J2, t2;

	int i, j, B, cid = (long) arg;
	int votes[N_ATTACKS][256];
	//first: first S-Box Setup; first2:first round with new key; oldB: old B value
	int first=1, first2=1, oldB=0, oldq=0;

	memcpy( S,  R, 256 );
	memcpy( Si, R, 256 );
	while( 1 )
	{
		if(!first) oldB=B;

		if( safe_read( mc_pipe[cid][0], (void *) &B,
			sizeof( int ) ) != sizeof( int ) )
		{
			perror( "read failed" );
			
			return -1;
		}
		if( close_aircrack )
			break;

		first2=1;

		min = 5 * ( ( (     cid ) * wep.nb_ivs ) / opt.nbcpu );
		max = 5 * ( ( ( 1 + cid ) * wep.nb_ivs ) / opt.nbcpu );

		q = 3 + B;

		memcpy( K + 3, wep.key, B );
		memset( votes, 0, sizeof( votes ) );

		/* START: KoreK attacks */

		for( xv = min; xv < max; xv += 5 )
		{
			if(!first)
			{
				for(i=0; i<oldq; i++)
				{
					if(stop_att)
						return -1;
					S[i] = Si[i] = i;
					S[jj[i]] = Si[jj[i]] = jj[i];
// 					Si[i] = i;
// 					Si[jj[i]] = jj[i];
				}
			}

			pthread_mutex_lock( &mx_ivb );

			memcpy( K, &wep.ivbuf[xv], 3 );

			for( i = j = 0; i < q; i++ )
			{
				if(stop_att)
					return -1;
//				i can never be 3+opt.keylen or exceed it, as i runs from 0 to q and q is defined as 3+B (with B the keybyte to attack)
// 				jj[i] = j = ( j + S[i] + K[i % (3 + opt.keylen)] ) & 0xFF;
				jj[i] = j = ( j + S[i] + K[i] ) & 0xFF;
				SWAP( S[i], S[j] );
			}

			i = q; do { i--; SWAP(Si[i],Si[jj[i]]); }
			while( i != 0 );

			o1 = wep.ivbuf[xv + 3] ^ 0xAA; io1 = Si[o1]; S1 = S[1];
			o2 = wep.ivbuf[xv + 4] ^ 0xAA; io2 = Si[o2]; S2 = S[2];
			pthread_mutex_unlock( &mx_ivb );

			if(first)
				first=0;
			if(first2)
			{
				oldB=B;
				oldq = 3+oldB;
				first2=0;
			}

			Sq = S[q]; dq = Sq + jj[q - 1];

			if( S2 == 0 )
			{
				if( ( S1 == 2 ) && ( o1 == 2 ) )
				{
					Kq = 1 - dq; votes[A_neg][Kq]++;
					Kq = 2 - dq; votes[A_neg][Kq]++;
					//to signal general usage
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
					//to know which attack used this iv
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_neg);
				}
				else if( o2 == 0 )
				{
					Kq = 2 - dq; votes[A_neg][Kq]++;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_neg);
				}
			}
			else
			{
				if( ( o2 == 0 ) && ( Sq == 0 ) )
				{
					Kq = 2 - dq; votes[A_u15][Kq]++;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_u15);
				}
			}

			if( ( S1 == 1 ) && ( o1 == S2 ) )
			{
				Kq = 1 - dq; votes[A_neg][Kq]++;
				Kq = 2 - dq; votes[A_neg][Kq]++;
// 				all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 				all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_neg);
			}

			if( ( S1 == 0 ) && ( S[0] == 1 ) && ( o1 == 1 ) )
			{
				Kq = 0 - dq; votes[A_neg][Kq]++;
				Kq = 1 - dq; votes[A_neg][Kq]++;
// 				all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 				all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_neg);
			}

			if( S1 == q )
			{
				if( o1 == q )
				{
					Kq = Si[0] - dq; votes[A_s13][Kq]++;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_s13);
				}
				else if( ( ( 1 - q - o1 ) & 0xFF ) == 0 )
				{
					Kq = io1 - dq; votes[A_u13_1][Kq]++;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_u13_1);
				}
				else if( io1 < q )
				{
					jq = Si[( io1 - q ) & 0xFF];

					if( jq != 1 )
					{
						Kq = jq - dq; votes[A_u5_1][Kq]++;
// 						all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 						all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_u5_1);
					}
				}
			}

			if( ( io1 == 2 ) && ( S[q] == 1 ) )
			{
				Kq = 1 - dq; votes[A_u5_2][Kq]++;
// 				all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 				all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_u5_2);
			}

			if( S[q] == q )
			{
				if( ( S1 == 0 ) && ( o1 == q ) )
				{
					Kq = 1 - dq; votes[A_u13_2][Kq]++;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_u13_2);
				}
				else if( ( ( ( 1 - q - S1 ) & 0xFF ) == 0 ) && ( o1 == S1 ) )
				{
					Kq = 1 - dq; votes[A_u13_3][Kq]++;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_u13_3);
				}
				else if( ( S1 >= ( ( -q ) & 0xFF ) )
					&& ( ( ( q + S1 - io1 ) & 0xFF ) == 0 ) )
				{
					Kq = 1 - dq; votes[A_u5_3][Kq]++;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_u5_3);
				}
			}

			if( ( S1 < q ) && ( ( ( S1 + S[S1] - q ) & 0xFF ) == 0 )  &&
				( io1 != 1 ) && ( io1 != S[S1] ) )
			{
				Kq = io1 - dq; votes[A_s5_1][Kq]++;
// 				all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 				all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_s5_1);
			}

			if( ( S1 > q ) && ( ( ( S2 + S1 - q ) & 0xFF ) == 0 ) )
			{
				if( o2 == S1 )
				{
					jq = Si[(S1 - S2) & 0xFF];

					if( ( jq != 1 ) && ( jq != 2 ) )
					{
						Kq = jq - dq; votes[A_s5_2][Kq]++;
// 						all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 						all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_s5_2);
					}
				}
				else if( o2 == ( ( 2 - S2 ) & 0xFF ) )
				{
					jq = io2;

					if( ( jq != 1 ) && ( jq != 2 ) )
					{
						Kq = jq - dq; votes[A_s5_3][Kq]++;
// 						all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 						all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_s5_3);
					}
				}
			}

			if( ( S[1] != 2 ) && ( S[2] != 0 ) )
			{
				J2 = S[1] + S[2];

				if( J2 < q )
				{
					t2 = S[J2] + S[2];

					if( ( t2 == q ) && ( io2 != 1 ) && ( io2 != 2 )
						&& ( io2 != J2 ) )
					{
						Kq = io2 - dq; votes[A_s3][Kq]++;
// 						all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 						all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_s3);
					}
				}
			}

			if( S1 == 2 )
			{
				if( q == 4 )
				{
					if( o2 == 0 )
					{
						Kq = Si[0] - dq; votes[A_4_s13][Kq]++;
// 						all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 						all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_4_s13);
					}
					else
					{
						if( ( jj[1] == 2 ) && ( io2 == 0 ) )
						{
							Kq = Si[254] - dq; votes[A_4_u5_1][Kq]++;
// 							all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 							all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_4_u5_1);
						}
						if( ( jj[1] == 2 ) && ( io2 == 2 ) )
						{
							Kq = Si[255] - dq; votes[A_4_u5_2][Kq]++;
// 							all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 							all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_4_u5_2);
						}
					}
				}
				else if( ( q > 4 ) && ( ( S[4] + 2 ) == q ) &&
					( io2 != 1 ) && ( io2 != 4 ) )
				{
					Kq = io2 - dq; votes[A_u5_4][Kq]++;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= USE_IV;
// 					all_ivs[256*256*K[0] + 256*K[1] + K[2]].used |= 1 << (4+A_u5_4);
				}
			}
			if( close_aircrack )
				break;
		}
		if( close_aircrack )
			break;
		/* END: KoreK attacks */

		if( safe_write( cm_pipe[cid][1], votes,
			sizeof( votes ) ) != sizeof( votes ) )
		{
			perror( "write failed" );
			kill( 0, SIGTERM );
			_exit( FAILURE );
		}
	}

	return( 0 );
}

/* display the current votes */

void show_wep_stats( int B, int force, PTW_tableentry table[PTW_KEYHSBYTES][PTW_n], int choices[KEYHSBYTES], int depth[KEYHSBYTES], int prod )
{
	float delta;
	struct winsize ws;
	int i, et_h, et_m, et_s;
	static int is_cleared = 0;

	if( (chrono( &t_stats, 0 ) < 1.51 || wepkey_crack_success) && force == 0 )
		return;

	if( ioctl( 0, TIOCGWINSZ, &ws ) < 0 )
	{
		ws.ws_row = 25;
		ws.ws_col = 80;
	}

	chrono( &t_stats, 1 );

	delta = chrono( &t_begin, 0 );

	et_h =   delta / 3600;
	et_m = ( delta - et_h * 3600 ) / 60;
	et_s =   delta - et_h * 3600 - et_m * 60;

	if( is_cleared == 0 )
	{
		is_cleared++;

		if( opt.l33t )
			printf( "\33[40m" );

		printf( "\33[2J" );
	}

	if( opt.l33t )
		printf( "\33[34;1m" );

	printf( "\33[2;%dH%s\n\n", (ws.ws_col - 12) / 2,
		progname );

	if( opt.l33t )
		printf( "\33[33;1m" );

	if(table)
		printf( "\33[5;%dH[%02d:%02d:%02d] Tested %d keys (got %ld IVs)\33[K",
			(ws.ws_col - 44) / 2, et_h, et_m, et_s, prod, opt.ap->nb_ivs );
	else
		printf( "\33[5;%dH[%02d:%02d:%02d] Tested %lld keys (got %ld IVs)\33[K",
			(ws.ws_col - 44) / 2, et_h, et_m, et_s, nb_tried, wep.nb_ivs_now );

	if( opt.l33t )
		printf( "\33[32;22m" );

	printf( "\33[7;4HKB    depth   byte(vote)\n" );

	for( i = 0; i <= B; i++ )
	{
		int j, k = ( ws.ws_col - 20 ) / 11;

		if(!table)
		{
			if( opt.l33t )
				printf( "   %2d  \33[1m%3d\33[22m/%3d   ",
					i, wep.depth[i], wep.fudge[i] );
			else
				printf( "   %2d  %3d/%3d   ",
					i, wep.depth[i], wep.fudge[i] );
		}
		else
			printf( "   %2d  %3d/%3d   ",
				i, depth[i], choices[i] );

		if(table)
		{
			for( j = depth[i]; j < k + depth[i]; j++ )
			{
				if( j >= 256 ) break;

				if( opt.l33t )
					printf( "\33[1m%02X\33[22m(%4d) ",
						table[i][j].b,
						table[i][j].votes );
				else
					printf( "%02X(%4d) ",  table[i][j].b,
						table[i][j].votes );
			}
		}
		else
		{
			for( j = wep.depth[i]; j < k + wep.depth[i]; j++ )
			{
				if( j >= 256 ) break;

				if( wep.poll[i][j].val == 32767 )
				{
					if( opt.l33t )
						printf( "\33[1m%02X\33[22m(+inf) ",
							wep.poll[i][j].idx );
					else
						printf( "%02X(+inf) ", wep.poll[i][j].idx );
				}
				else
				{
					if( opt.l33t )
						printf( "\33[1m%02X\33[22m(%4d) ",
							wep.poll[i][j].idx,
							wep.poll[i][j].val );
					else
						printf( "%02X(%4d) ",  wep.poll[i][j].idx,
							wep.poll[i][j].val );
				}
			}
		}
		if (opt.showASCII && !table)
			if(wep.poll[i][wep.depth[i]].idx>=ASCII_LOW_T && wep.poll[i][wep.depth[i]].idx<=ASCII_HIGH_T)
				if(wep.poll[i][wep.depth[i]].val>=ASCII_VOTE_STRENGTH_T || ASCII_DISREGARD_STRENGTH )
					printf( "  %c",wep.poll[i][wep.depth[i]].idx );

		printf( "\n" );
	}

	if( B < opt.keylen - 1 )
		printf( "\33[J" );

	printf( "\n" );
}

static void key_found(unsigned char *wepkey, int keylen, int B)
{
	FILE * keyFile;
	int i, n;
	int nb_ascii = 0;
	JNIEnv* env;

	int sec_thread = (*jvm)->AttachCurrentThread(jvm, &env, NULL);
	if(sec_thread < 0)
		env = cb_jnienv;

	for( i = 0; i < keylen; i++ )
		if( wepkey[i] == 0 ||
		( wepkey[i] >= 32 && wepkey[i] < 127 ) )
			nb_ascii++;

	wepkey_crack_success = 1;
	memcpy(bf_wepkey, wepkey, keylen);

	if( opt.is_quiet ){
		printf( "KEY FOUND! [ " );
		snprintf(info_buffer, sizeof(info_buffer),  "KEY FOUND! [ "  );
    	info_message_thread(info_buffer, UPDATE_SUCCESS, env);
	}
	
	else
	{
		if (B != -1)
			show_wep_stats( B - 1, 1, NULL, NULL, NULL, 0 );

		if( opt.l33t )
			printf( "\33[31;1m" );

		n = ( 80 - 14 - keylen * 3 ) / 2;

		if( 100 * nb_ascii > 75 * keylen )
			n -= ( keylen + 4 ) / 2;

		if( n <= 0 ) n = 0;

		printf( "\33[K\33[%dCKEY FOUND! [ ", n );
	}

	for( i = 0; i < keylen - 1; i++ ) {
		printf( "%02X:", wepkey[i] );
		snprintf(info_buffer, sizeof(info_buffer), "%02X:", wepkey[i] );
    	info_message_thread(info_buffer, UPDATE_SUCCESS, env);
	}
	printf( "%02X ] ",   wepkey[i] );
	snprintf(info_buffer, sizeof(info_buffer), "%02X ] ",   wepkey[i]  );
    	info_message_thread(info_buffer, UPDATE_SUCCESS, env);
	
	if( nb_ascii == keylen )
	{
		printf( "(ASCII: " );
		snprintf(info_buffer, sizeof(info_buffer), "(ASCII: "  );
    	info_message_thread(info_buffer, UPDATE_SUCCESS, env);

		for( i = 0; i < keylen; i++ ) {
			printf( "%c", ( ( wepkey[i] >  31 && wepkey[i] < 127 ) ||
				wepkey[i] > 160 ) ? wepkey[i] : '.' );
			snprintf(info_buffer, sizeof(info_buffer),"%c", ( ( wepkey[i] >  31 && wepkey[i] < 127 ) ||
				wepkey[i] > 160 ) ? wepkey[i] : '.' );
    	info_message_thread(info_buffer, UPDATE_SUCCESS, env);
           
		}
		snprintf(info_buffer, sizeof(info_buffer)," )\n" );
    	info_message_thread(info_buffer, UPDATE_SUCCESS, env);
		printf( " )" );
	}

	if( opt.l33t )
		printf( "\33[32;22m" );

	printf( "\n\tDecrypted correctly: %d%%\n", opt.probability );
	printf( "\n" );

	snprintf(info_buffer, sizeof(info_buffer),"\n\tDecrypted correctly: %d%%\n\n", opt.probability );
    	info_message_thread(info_buffer, UPDATE_SUCCESS, env);

	// Write the key to a file
	if (opt.logKeyToFile != NULL) {
		keyFile = fopen(opt.logKeyToFile, "w");
		if (keyFile != NULL)
		{
			for( i = 0; i < keylen; i++ )
				fprintf(keyFile, "%02X", wepkey[i]);
			fclose(keyFile);
		}
	}

	if(sec_thread >= 0 && opt.do_ptw == 0)
		(*jvm)->DetachCurrentThread(jvm);

}

/* test if the current WEP key is valid */

int check_wep_key( unsigned char *wepkey, int B, int keylen )
{
	unsigned char x1, x2;
	unsigned long xv;
	int i, j, n, bad, tests;

	unsigned char K[64];
	unsigned char S[256];

	if (keylen<=0)
		keylen = opt.keylen;

	pthread_mutex_lock(&mx_nb);
	nb_tried++;
	pthread_mutex_unlock(&mx_nb);

	bad = 0;

	memcpy( K + 3, wepkey, keylen );

	tests = 32;

// 	printf("keylen: %d\n", keylen);
// 	if(keylen==13)
// 		printf("%02X:%02X:%02X:%02X:%02X\n", wepkey[8],wepkey[9],wepkey[10],wepkey[11],wepkey[12]);

	if(opt.dict) tests = wep.nb_ivs;

	if(tests < TEST_MIN_IVS) tests=TEST_MIN_IVS;
	if(tests > TEST_MAX_IVS) tests=TEST_MAX_IVS;

	for( n = 0; n < tests; n++ )
	{
		if(stop_att)
			return FAILURE;
		/* xv = 5 * ( rand() % wep.nb_ivs ); */
		xv = 5 * n;

		pthread_mutex_lock( &mx_ivb );

		memcpy( K, &wep.ivbuf[xv], 3 );
		memcpy( S, R, 256 );

		for( i = j = 0; i < 256; i++ )
		{
			if(stop_att)
				return FAILURE;
			j = ( j + S[i] + K[i % (3 + keylen)]) & 0xFF;
			SWAP( S[i], S[j] );
		}

		i = 1; j = ( 0 + S[i] ) & 0xFF; SWAP(S[i], S[j]);
		x1 = wep.ivbuf[xv + 3] ^ S[(S[i] + S[j]) & 0xFF];

		i = 2; j = ( j + S[i] ) & 0xFF; SWAP(S[i], S[j]);
		x2 = wep.ivbuf[xv + 4] ^ S[(S[i] + S[j]) & 0xFF];

		pthread_mutex_unlock( &mx_ivb );

//		printf("xv: %li x1: %02X  x2: %02X\n", (xv/5), x1, x2);

		if( ( x1 != 0xAA || x2 != 0xAA ) &&
			( x1 != 0xE0 || x2 != 0xE0 ) &&
			( x1 != 0x42 || x2 != 0x42 ) &&
			( x1 != 0x02 || x2 != 0xAA ) )					//llc sub layer management
			bad++;

		if( bad > ((tests*opt.probability)/100) )
			return( FAILURE );
	}

	opt.probability = (((tests-bad)*100)/tests);
	key_found(wepkey, keylen, B);

	return( SUCCESS );
}

/* routine used to sort the votes */

int cmp_votes( const void *bs1, const void *bs2 )
{
	if( ((vote *) bs1)->val < ((vote *) bs2)->val )
		return(  1 );

	if( ((vote *) bs1)->val > ((vote *) bs2)->val )
		return( -1 );

	return( 0 );
}

/* sum up the votes and sort them */

int calc_poll( int B )
{
	int i, n, cid, *vi;
	int votes[N_ATTACKS][256];

	memset(&opt.votes, '\0', sizeof(opt.votes));

	/* send the current keybyte # to each thread */

	for( cid = 0; cid < opt.nbcpu; cid++ )
	{
		n = sizeof( int );
		if(stop_att)
			return -1;
		if( safe_write( mc_pipe[cid][1], &B, n ) != n )
		{
			perror( "write failed" );
			
			return -1;
			//kill( 0, SIGTERM );
			//_exit( FAILURE );
		}
	}

	/* collect the votes, multiply by the korek coeffs */

	for( i = 0; i < 256; i++ )
	{
		if(stop_att)
			return -1;
		wep.poll[B][i].idx = i;
		wep.poll[B][i].val = 0;
	}

	for( cid = 0; cid < opt.nbcpu; cid++ )
	{
		n = sizeof( votes );

		if( safe_read( cm_pipe[cid][0], votes, n ) != n )
		{
			perror( "read failed" );
			kill( 0, SIGTERM );
			_exit( FAILURE );
		}

		for( n = 0, vi = (int *) votes; n < N_ATTACKS; n++ )
			for( i = 0; i < 256; i++, vi++ )
			{
				if(stop_att)
					return -1;
				wep.poll[B][i].val += *vi * K_COEFF[n];
				if(K_COEFF[n]) opt.votes[n] += *vi;
			}
	}

	/* set votes to the max if the keybyte is user-defined */

	if( opt.debug_row[B] )
		wep.poll[B][opt.debug[B]].val = 32767;

	/* if option is set, restrict keyspace to alpha-numeric */

	if( opt.is_alnum )
	{
		for( i = 1; i < 32; i++ )
			wep.poll[B][i].val = -1;

		for( i = 127; i < 256; i++ )
			wep.poll[B][i].val = -1;
	}

	if( opt.is_fritz )
	{
		for( i = 0; i < 48; i++ )
			wep.poll[B][i].val = -1;

		for( i = 58; i < 256; i++ )
			wep.poll[B][i].val = -1;
	}

	/* if option is set, restrict keyspace to BCD hex digits */

	if( opt.is_bcdonly )
	{
		for( i = 1; i < 256; i++ )
			if( i > 0x99 || ( i & 0x0F ) > 0x09 )
				wep.poll[B][i].val = -1;
	}

	/* sort the votes, highest ones first */

	qsort( wep.poll[B], 256, sizeof( vote ), cmp_votes );

	return( SUCCESS );
}

int update_ivbuf( void )
{
	int n;
	struct AP_info *ap_cur;

	/* 1st pass: compute the total number of available IVs */

	wep.nb_ivs_now = 0;
	wep.nb_aps = 0;
	ap_cur = ap_1st;

	while( ap_cur != NULL )
	{
		if(stop_att)
			return FAILURE;
		if( ap_cur->crypt == 2 && ap_cur->target )
		{
			wep.nb_ivs_now += ap_cur->nb_ivs;
			wep.nb_aps++;
		}

		ap_cur = ap_cur->next;
	}

	/* 2nd pass: create the main IVs buffer if necessary */

	if( wep.nb_ivs == 0 ||
		( opt.keylen ==  5 && wep.nb_ivs_now - wep.nb_ivs > 20000 ) ||
		( opt.keylen >= 13 && wep.nb_ivs_now - wep.nb_ivs > 40000 ) )
	{
		/* one buffer to rule them all */

		pthread_mutex_lock( &mx_ivb );

		if( wep.ivbuf != NULL )
		{
			free( wep.ivbuf );
			wep.ivbuf = NULL;
		}

		wep.nb_ivs = 0;

		ap_cur = ap_1st;

		while( ap_cur != NULL )
		{
			if(stop_att)
				return FAILURE;
			if( ap_cur->crypt == 2 && ap_cur->target )
			{
				n = ap_cur->nb_ivs;

				if( ( wep.ivbuf = realloc( wep.ivbuf,
					( wep.nb_ivs + n ) * 5 ) ) == NULL )
				{
					pthread_mutex_unlock( &mx_ivb );
					perror( "realloc failed" );
					return FAILURE;
				}

				memcpy( wep.ivbuf + wep.nb_ivs * 5, ap_cur->ivbuf, 5 * n );

				wep.nb_ivs += n;
			}

			ap_cur = ap_cur->next;
		}

		pthread_mutex_unlock( &mx_ivb );
		if(stop_att)
			return FAILURE;
		return( RESTART );
	}

	return( SUCCESS );
}

/*
 * It will remove votes for a specific keybyte (and remove from the requested current value)
 * Return 0 on success, another value on failure
 */
int remove_votes(int keybyte, unsigned char value)
{
	int i;
	int found = 0;
	for (i=0; i < 256; i++)
	{
		if(stop_att)
			return -1;
		if (wep.poll[keybyte][i].idx == (int)value)
		{
			found = 1;
			//wep.poll[keybyte][i].val = 0;
			// Update wep.key
		}
		if (found)
		{
			// Put the value at the end with NO votes
			if (i== 255)
			{
				wep.poll[keybyte][i].idx = (int)value;
				wep.poll[keybyte][i].val = 0;
			}
			else
			{
				wep.poll[keybyte][i].idx = wep.poll[keybyte][i + 1].idx;
				wep.poll[keybyte][i].val = wep.poll[keybyte][i + 1].val;
				if (i == 0)
				{
					// Also update wep key if it's the first value to remove
					wep.key[keybyte] = wep.poll[keybyte][i].idx;
				}
			}
		}
	}
	return 0;
}

/* standard attack mode: */
/* this routine gathers and sorts the votes, then recurses until it *
 * reaches B == keylen. It also stops when the current keybyte vote *
 * is lower than the highest vote divided by the fudge factor.      */

int do_wep_crack1( int B )
{
	int i, j, l, m, tsel, charread, askchange;
	int remove_keybyte_nr, remove_keybyte_value;
	//int a,b;
	static int k = 0;
	char user_guess[4];

	askchange = 1;

	get_ivs:
	if(stop_att)
		return FAILURE;
	switch( update_ivbuf() )
	{
		case FAILURE: return( FAILURE );
		case RESTART: return( RESTART );
		default: break;
	}
	if(stop_att)
		return FAILURE;
	if( ( wep.nb_ivs_now < 256 && opt.debug[0] == 0 ) ||
		( wep.nb_ivs_now <  32 && opt.debug[0] != 0 ) )
	{
		if( ! opt.no_stdin )
		{
			printf(
				"Not enough IVs available. You need about 250 000 IVs to crack\n"
				"40-bit WEP, and more than 800 000 IVs to crack a 104-bit key.\n" );
			snprintf(info_buffer, sizeof(info_buffer),"Not enough IVs available. You need about 250 000 IVs to crack\n"
				"40-bit WEP, and more than 800 000 IVs to crack a 104-bit key.\n");
                	info_message(info_buffer, UPDATE_ERROR);
			//kill( 0, SIGTERM );
			_exit( FAILURE );

		}
		else
		{
			printf( "Read %ld packets, got %ld IVs...\r",
				nb_pkt, wep.nb_ivs_now );
			fflush( stdout );
			snprintf(info_buffer, sizeof(info_buffer),"Read %ld packets, got %ld IVs...\r",
				nb_pkt, wep.nb_ivs_now );
                	info_message(info_buffer, UPDATE_ERROR);
			sleep( 1 );
			goto get_ivs;
		}
	}

	/* if last keybyte reached, check if the key is valid */

	if( B == opt.keylen )
	{
		if( ! opt.is_quiet )
			show_wep_stats( B - 1, 0, NULL, NULL, NULL, 0 );

		return( check_wep_key( wep.key, B, 0 ) );
	}

	/* now compute the poll resultst for keybyte B */

	if( calc_poll( B ) != SUCCESS )
		return( FAILURE );

	/* fudge threshold = higest vote divided by fudge factor */

	for( wep.fudge[B] = 1; wep.fudge[B] < 256; wep.fudge[B]++ ) {
		if(stop_att)
		return FAILURE;
	if( (float) wep.poll[B][wep.fudge[B]].val <
		(float) wep.poll[B][0].val / opt.ffact )
			break;
	}
		

	/* try the most likely n votes, where n is the fudge threshold */

	for( wep.depth[B] = 0; wep.depth[B] < wep.fudge[B]; ( wep.depth[B] )++ )
	{
		switch( update_ivbuf() )
		{
			case FAILURE: return( FAILURE );
			case RESTART: return( RESTART );
			default: break;
		}
		if(stop_att)
		return FAILURE;

		wep.key[B] = wep.poll[B][wep.depth[B]].idx;

		if( ! opt.is_quiet )
		{
			show_wep_stats( B, 0, NULL, NULL, NULL, 0 );
		}

		if( B == 4 && opt.keylen == 13 )
		{
			/* even when cracking 104-bit WEP, *
			 * check if the 40-bit key matches */

			/* opt.keylen = 5; many functions use keylen. it is dangerous to do this in a multithreaded process */

			if( check_wep_key( wep.key, B, 5 ) == SUCCESS )
			{
				opt.keylen = 5;
				return( SUCCESS );
			}

			/* opt.keylen = 13; */
		}



		if( B + opt.do_brute + 1 == opt.keylen && opt.do_brute )
		{
			/* as noted by Simon Marechal, it's more efficient
			 * to just bruteforce the last two keybytes. */

			/*
				Ask for removing votes here
				1. Input keybyte. Use enter when it's done => Bruteforce will start
				2. Input value to remove votes from: 00 -> FF or Enter to cancel remove
				3. Remove votes
				4. Redraw
				5. Go back to 1
			*/
			if (opt.visual_inspection == 1)
			{
				while(1)
				{
					if(stop_att)
						return -1;
					// Show the current stat
					show_wep_stats( B, 1, NULL, NULL, NULL, 0 );

					// Inputting user value until it hits enter or give a valid value
					printf("On which keybyte do you want to remove votes (Hit Enter when done)? ");
					memset(user_guess, 0, 4);

					charread = readLine(user_guess, 3);

					// Break if 'Enter' key was hit
					if (user_guess[0] == 0 || charread == 0)
						break;

					// If it's not a number, reask
					// Check if inputted value is correct (from 0 to and inferior to opt.keylen)
					remove_keybyte_nr = atoi(user_guess);
					if (isdigit((int)user_guess[0]) == 0 || remove_keybyte_nr < 0 || remove_keybyte_nr >= opt.keylen)
						continue;


					// It's a number for sure and the number is correct
					// Now ask which value should be removed
					printf("From which keybyte value do you want to remove the votes (Hit Enter to cancel)? ");
					memset(user_guess, 0, 4);
					charread = readLine(user_guess, 3);

					// Break if enter was hit
					if (user_guess[0] == 0 || charread == 0)
						continue;

					remove_keybyte_value = hexToInt(user_guess, charread);

					// Check if inputted value is correct (hexa). Value range: 00 - FF
					if (remove_keybyte_value < 0 || remove_keybyte_value > 255)
						continue;

					// If correct, remove and redraw
					remove_votes(remove_keybyte_nr, (unsigned char)remove_keybyte_value);
				}
			}
			if (opt.nbcpu==1 || opt.do_mt_brute==0)
			{

				if (opt.do_brute==4)
				{
					for( l = 0; l < 256; l++)
					{
						wep.key[opt.brutebytes[0]] = l;

						for( m = 0; m < 256; m++ )
						{
							wep.key[opt.brutebytes[1]] = m;

							for( i = 0; i < 256; i++ )
							{
								wep.key[opt.brutebytes[2]] = i;

								for( j = 0; j < 256; j++ )
								{
									if(stop_att)
										return FAILURE;
									wep.key[opt.brutebytes[3]] = j;

									if (check_wep_key( wep.key, B + 1, 0 ) == SUCCESS)
										return SUCCESS;
								}
							}
						}
					}
				}
				else if (opt.do_brute==3)
				{
					for( m = 0; m < 256; m++ )
					{
						wep.key[opt.brutebytes[0]] = m;

						for( i = 0; i < 256; i++ )
						{
							wep.key[opt.brutebytes[1]] = i;

							for( j = 0; j < 256; j++ )
							{
								if(stop_att)
									return FAILURE;
								wep.key[opt.brutebytes[2]] = j;

								if (check_wep_key( wep.key, B + 1, 0 ) == SUCCESS)
									return SUCCESS;
								if(stop_att)
									return FAILURE;
							}
						}
					}
				}
				else if (opt.do_brute==2)
				{
					for( i = 0; i < 256; i++ )
					{
						wep.key[opt.brutebytes[0]] = i;

						for( j = 0; j < 256; j++ )
						{
							if(stop_att)
								return FAILURE;
							wep.key[opt.brutebytes[1]] = j;

							if (check_wep_key( wep.key, B + 1, 0 ) == SUCCESS)
								return SUCCESS;
						}
					}
				}
				else
				{
					for( i = 0; i < 256; i++ )
					{
						if(stop_att)
							return FAILURE;
						wep.key[opt.brutebytes[0]] = i;

						if (check_wep_key( wep.key, B + 1, 0 ) == SUCCESS)
							return SUCCESS;
					}
				}
			}
			else
			{
				/* multithreaded bruteforcing of the last 2 keybytes */
				k = (k+1) % opt.nbcpu;
				do
				{
					for(tsel=0; tsel<opt.nbcpu && !wepkey_crack_success; ++tsel)
					{
						if (bf_nkeys[(tsel+k) % opt.nbcpu]>16)
						{
							if(stop_att)
								return FAILURE;
							usleep(1);
							continue;
						}
						else
						{
							if(stop_att)
								return FAILURE;
							/* write our current key to the pipe so it'll have its last 2 bytes bruteforced */
							bf_nkeys[(tsel+k) % opt.nbcpu]++;

							if (safe_write(bf_pipe[(tsel+k) % opt.nbcpu][1], (void *) wep.key, 64) != 64)
							{
								perror( "write pmk failed" );
								//kill( 0, SIGTERM );
								//_exit( FAILURE );
								
								return -1;
							}
							break;
						}
					}
					if(stop_att)
						return FAILURE;
				} while (tsel>=opt.nbcpu && !wepkey_crack_success);
				if(stop_att)
					return FAILURE;
				if (wepkey_crack_success)
				{
					memcpy(wep.key, bf_wepkey, opt.keylen);
					return(SUCCESS);
				}
			}
		}
		else
		{
			if(stop_att)
				return FAILURE;
			switch( do_wep_crack1( B + 1 ) )
			{
				case SUCCESS: return( SUCCESS );
				case RESTART: return( RESTART );
				default: break;
			}
		}
	}

	//if we are going to fail on the root byte, check again if there are still threads bruting, if so wait and check again.
	if(B==0)
	{
		for(i=0; i<opt.nbcpu; i++)
		{
			while(bf_nkeys[i]>0 && !wepkey_crack_success) {
				usleep(1);
				if(stop_att)
					return FAILURE;
			}
		}
		if (wepkey_crack_success)
		{
			memcpy(wep.key, bf_wepkey, opt.keylen);
			return(SUCCESS);
		}
		if(stop_att)
			return FAILURE;
	}
	return( FAILURE );
}

/* experimental single bruteforce attack */

int do_wep_crack2( int B )
{
	int i, j;

	switch( update_ivbuf() )
	{
		case FAILURE: return( FAILURE );
		case RESTART: return( RESTART );
		default: break;
	}

	if( wep.nb_ivs_now / opt.keylen < 60000 )
	{
		printf(
			"Not enough IVs available. This option is only meant to be used\n"
			"if the standard attack method fails with more than %d IVs.\n",
			opt.keylen * 60000 );
		snprintf(info_buffer, sizeof(info_buffer), "Not enough IVs available. This option is only meant to be used\n"
			"if the standard attack method fails with more than %d IVs.\n",
			opt.keylen * 60000 );
                	info_message(info_buffer, UPDATE_ERROR);

		//kill( 0, SIGTERM );
		_exit( FAILURE );
	}

	for( i = 0; i <= B; i++ )
	{
		if(stop_att)
			return -1;
		if( calc_poll( i ) != SUCCESS )
			return( FAILURE );

		wep.key[i] = wep.poll[i][0].idx;

		wep.fudge[i] = 1;
		wep.depth[i] = 0;

		if( ! opt.is_quiet )
			show_wep_stats( i, 0, NULL, NULL, NULL, 0 );
	}

	for( wep.fudge[B] = 1; wep.fudge[B] < 256; wep.fudge[B]++ )
		if( (float) wep.poll[B][wep.fudge[B]].val <
		(float) wep.poll[B][0].val / opt.ffact )
			break;

	for( wep.depth[B] = 0; wep.depth[B] < wep.fudge[B]; wep.depth[B]++ )
	{
		switch( update_ivbuf() )
		{
			case FAILURE: return( FAILURE );
			case RESTART: return( RESTART );
			default: break;
		}

		wep.key[B] = wep.poll[B][wep.depth[B]].idx;

		if( ! opt.is_quiet )
			show_wep_stats( B, 0, NULL, NULL, NULL, 0 );

		for( i = B + 1; i < opt.keylen - 2; i++ )
		{

			if(stop_att)
				return -1;
			if( calc_poll( i ) != SUCCESS )
				return( FAILURE );

			wep.key[i] = wep.poll[i][0].idx;

			wep.fudge[i] = 1;
			wep.depth[i] = 0;

			if( ! opt.is_quiet )
				show_wep_stats( i, 0, NULL, NULL, NULL, 0 );
		}

		for( i = 0; i < 256; i++ )
		{
			wep.key[opt.keylen - 2] = i;

			for( j = 0; j < 256; j++ )
			{
				if(stop_att)
					return -1;
				wep.key[opt.keylen - 1] = j;

				if( check_wep_key( wep.key, opt.keylen - 2, 0 ) == SUCCESS )
					return( SUCCESS );
			}
		}
	}

	return( FAILURE );
}

int inner_bruteforcer_thread(void *arg)
{
	int i, j, k, l, reduce=0;
	size_t nthread = (size_t)arg;
	unsigned char wepkey[64];
	int ret=0;

	inner_bruteforcer_thread_start:

	reduce=0;

	if( close_aircrack )
		return(ret);

	if (wepkey_crack_success)
		return(SUCCESS);

	/* we get the key for which we'll bruteforce the last 2 bytes from the pipe */
	if( safe_read( bf_pipe[nthread][0], (void *) wepkey, 64) != 64)
	{
		perror( "read failed" );
		kill( 0, SIGTERM );
		_exit( FAILURE );
	}
	else
		reduce=1;

	if( close_aircrack )
		return(ret);
	/* now we test the 256*256 keys... if we succeed we'll save it and exit the thread */
	if (opt.do_brute==4)
	{
		for( l = 0; l < 256; l++ )
		{
			wepkey[opt.brutebytes[0]] = l;

			for( k = 0; k < 256; k++ )
			{
				wepkey[opt.brutebytes[1]] = k;

				for( i = 0; i < 256; i++ )
				{
					wepkey[opt.brutebytes[2]] = i;

					for( j = 0; j < 256; j++ )
					{
						if(stop_att)
							return -1;
						wepkey[opt.brutebytes[3]] = j;

						if( check_wep_key( wepkey, opt.keylen - 2, 0 ) == SUCCESS )
							return(SUCCESS);
					}
				}
			}
		}
	}
	else if (opt.do_brute==3)
	{
		for( k = 0; k < 256; k++ )
		{
			wepkey[opt.brutebytes[0]] = k;

			for( i = 0; i < 256; i++ )
			{
				wepkey[opt.brutebytes[1]] = i;

				for( j = 0; j < 256; j++ )
				{
					if(stop_att)
						return -1;
					wepkey[opt.brutebytes[2]] = j;

					if( check_wep_key( wepkey, opt.keylen - 2, 0 ) == SUCCESS )
						return(SUCCESS);
				}
			}
		}
	}
	else if (opt.do_brute==2)
	{
		for( i = 0; i < 256; i++ )
		{
			wepkey[opt.brutebytes[0]] = i;

			for( j = 0; j < 256; j++ )
			{
				if(stop_att)
					return -1;
				wepkey[opt.brutebytes[1]] = j;

				if( check_wep_key( wepkey, opt.keylen - 2, 0 ) == SUCCESS )
					return(SUCCESS);
			}
		}
	}
	else
	{
		for( j = 0; j < 256; j++ )
		{
			if(stop_att)
				return -1;
			wepkey[opt.brutebytes[0]] = j;

			if( check_wep_key( wepkey, opt.keylen - 2, 0 ) == SUCCESS )
				return(SUCCESS);
		}
	}

	if(reduce)
		bf_nkeys[nthread]--;

	goto inner_bruteforcer_thread_start;

}


/* display the current wpa key info, matrix-like */

void show_wpa_stats( char *key, int keylen, unsigned char pmk[32], unsigned char ptk[64],
unsigned char mic[16], int force )
{
	float delta, calc, ksec;
	int i, et_h, et_m, et_s;
	char tmpbuf[28];
	long long int remain, eta;

	if (chrono( &t_stats, 0 ) < 0.15 && force == 0)
		return;

	if (force != 0)
		pthread_mutex_lock(&mx_wpastats);  // if forced, wait until we can lock
	else
		if (pthread_mutex_trylock(&mx_wpastats) != 0)  // if not forced, just try
			return;

	chrono( &t_stats, 1 );

	delta = chrono( &t_begin, 0 );

	et_h =   delta / 3600;
	et_m = ( delta - et_h * 3600 ) / 60;
	et_s =   delta - et_h * 3600 - et_m * 60;

	if( ( delta = chrono( &t_kprev, 0 ) ) >= 6 )
	{
		int delta0;
		delta0 = delta;

		t_kprev.tv_sec += 3;
		delta = chrono( &t_kprev, 0 );
		nb_kprev *= delta / delta0;

	}

	if (_speed_test) {
		int ks = (int) ((float) nb_kprev / delta);

		printf("%d k/s\r", ks);
		fflush(stdout);

		if (et_s >= 15) {
			printf("\n");
			exit(0);
		}

		goto __out;
	}

	ksec = (float) nb_kprev / delta;

	if( opt.l33t ) printf( "\33[33;1m" );

	if (opt.stdin_dict) {
		printf( "\33[5;20H[%02d:%02d:%02d] %lld keys tested "
			"(%2.2f k/s) ", et_h, et_m, et_s,
			nb_tried, (float) nb_kprev / delta);
	} else {
		calc = ((float)nb_tried / (float)opt.wordcount)*100;
		remain = (opt.wordcount - nb_tried);
		eta = (remain / (long long int)ksec);

		printf( "\33[4;7H[%02d:%02d:%02d] %lld/%lld keys tested "
			"(%2.2f k/s) ", et_h, et_m, et_s,
			nb_tried, opt.wordcount, (float) nb_kprev / delta);

		printf( "\33[6;7HTime left: ");
		calctime(eta, calc);
	}

	memset( tmpbuf, ' ', sizeof( tmpbuf ) );
	memcpy( tmpbuf, key, keylen > 27 ? 27 : keylen );
	tmpbuf[27] = '\0';

	if( opt.l33t ) printf( "\33[37;1m" );
	printf( "\33[8;24HCurrent passphrase: %s\n", tmpbuf );

	if( opt.l33t ) printf( "\33[32;22m" );
	printf( "\33[11;7HMaster Key     : " );

	if( opt.l33t ) printf( "\33[32;1m" );
	for( i = 0; i < 32; i++ )
	{
		if( i == 16 ) printf( "\n\33[23C" );
		printf( "%02X ", pmk[i] );
	}

	if( opt.l33t ) printf( "\33[32;22m" );
	printf( "\33[14;7HTransient Key  : " );

	if( opt.l33t ) printf( "\33[32;1m" );
	for( i = 0; i < 64; i++ )
	{
		if( i > 0 && i % 16 == 0 ) printf( "\n\33[23C" );
		printf( "%02X ", ptk[i] );
	}

	if( opt.l33t ) printf( "\33[32;22m" );
	printf( "\33[19;7HEAPOL HMAC     : " );

	if( opt.l33t ) printf( "\33[32;1m" );
	for( i = 0; i < 16; i++ )
		printf( "%02X ", mic[i] );

	printf( "\n" );
__out:
	pthread_mutex_unlock(&mx_wpastats);
}


int do_make_wkp(struct AP_info *ap_cur)
{
	size_t elt_written;
	unsigned i = 0;

	while( ap_cur != NULL )
	{
		if( ap_cur->target && ap_cur->wpa.state == 7 )
			break;
		ap_cur = ap_cur->next;
	}

	if( ap_cur == NULL )
	{
		printf( "No valid WPA handshakes found.\n" );
		return( 0 );
	}

	if( memcmp( ap_cur->essid, ZERO, 32 ) == 0 && ! opt.essid_set )
	{
		printf( "An ESSID is required. Try option -e.\n" );
		return( 0 );
	}

	if( opt.essid_set && ap_cur->essid[0] == '\0' )
	{
		memset(  ap_cur->essid, 0, sizeof( ap_cur->essid ) );
		strncpy( ap_cur->essid, opt.essid, sizeof( ap_cur->essid ) - 1 );
	}

	printf("\n\nBuilding WKP (3.02) file...\n\n");

	printf("[*] ESSID (length: %d): %s\n", (int)strlen(ap_cur->essid), ap_cur->essid);

	printf("[*] Key version: %d\n", ap_cur->wpa.keyver);

	printf("[*] BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
		ap_cur->bssid[0], ap_cur->bssid[1],
		ap_cur->bssid[2], ap_cur->bssid[3],
		ap_cur->bssid[4], ap_cur->bssid[5]
		);
	printf("[*] STA: %02X:%02X:%02X:%02X:%02X:%02X",
		ap_cur->wpa.stmac[0], ap_cur->wpa.stmac[1],
		ap_cur->wpa.stmac[2], ap_cur->wpa.stmac[3],
		ap_cur->wpa.stmac[4], ap_cur->wpa.stmac[5]
		);

	printf("\n[*] anonce:");
	for(i = 0; i < 32; i++)
	{
		if(i % 16 == 0) printf("\n    ");
		printf("%02X ", ap_cur->wpa.anonce[i]);
	}

	printf("\n[*] snonce:");
	for(i = 0; i < 32; i++)
	{
		if(i % 16 == 0) printf("\n    ");
		printf("%02X ", ap_cur->wpa.snonce[i]);
	}

	printf("\n[*] Key MIC:\n   ");
	for(i = 0; i < 16; i++)
	{
		printf(" %02X", ap_cur->wpa.keymic[i]);
	}

	printf("\n[*] hsPackets:");
	for( i = 0; i < ap_cur->wpa.eapol_size; i++)
	{
		if( i % 16 == 0 ) printf("\n    ");
		printf("%02X ",ap_cur->wpa.eapol[i]);

	}

	printf("\n");

	// write file
	FILE * fp_wkp;
	char frametmp[WKP_FRAME_LENGTH];
	char *ptmp;

	memcpy(frametmp, wkp_frame,WKP_FRAME_LENGTH * sizeof(char));

	// Make sure the filename contains the extension
	if (( strstr(opt.wkp, ".wkp") == NULL || strlen(strstr(opt.wkp, ".wkp")) != 4 )
		 &&	( strstr(opt.wkp, ".WKP") == NULL || strlen(strstr(opt.wkp, ".WKP")) != 4) )
	{
		strcat(opt.wkp, ".wkp");
	}

	fp_wkp = fopen( opt.wkp,"w" );
	if (fp_wkp == NULL)
	{
		printf("\nFailed to create EWSA project file\n");
		return 0;
	}

	// ESSID
	memcpy(&frametmp[0x4c0], ap_cur->essid, sizeof(ap_cur->essid));


	// BSSID
	ptmp = (char *)ap_cur->bssid;
	memcpy(&frametmp[0x514], ptmp, 6);

	// Station Mac
	ptmp = (char *)ap_cur->wpa.stmac;
	memcpy(&frametmp[0x51a], ptmp, 6);

	// ESSID
	memcpy(&frametmp[0x520], ap_cur->essid, sizeof(ap_cur->essid));

	// ESSID length
	frametmp[0x540] = strlen(ap_cur->essid);

	// WPA Key version
	frametmp[0x544] = ap_cur->wpa.keyver;

	// Size of EAPOL
	frametmp[0x548] = ap_cur->wpa.eapol_size;

	// anonce
	ptmp = (char *)ap_cur->wpa.anonce;
	memcpy(&frametmp[0x54c], ptmp, 32);

	// snonce
	ptmp = (char *)ap_cur->wpa.snonce;
	memcpy(&frametmp[0x56c], ptmp, 32);

	// EAPOL
	ptmp = (char *)ap_cur->wpa.eapol;
	memcpy(&frametmp[0x58c], ptmp, ap_cur->wpa.eapol_size);

	// Key MIC
	ptmp = (char *)ap_cur->wpa.keymic;
	memcpy(&frametmp[0x68c], ptmp, 16);

	elt_written = fwrite(frametmp, 1, WKP_FRAME_LENGTH, fp_wkp);
	fclose(fp_wkp);


	if ((int)elt_written == WKP_FRAME_LENGTH) {
		printf("\nSuccessfully written to %s\n", opt.wkp);
	} else {
		printf("\nFailed to write to %s\n !", opt.wkp);
	}

	return( 1 );
}

int do_make_hccap(struct AP_info *ap_cur)
{
	size_t elt_written;
	unsigned i = 0;

	while( ap_cur != NULL )
	{
		if( ap_cur->target && ap_cur->wpa.state == 7 )
			break;
		ap_cur = ap_cur->next;
	}

	if( ap_cur == NULL )
	{
		printf( "No valid WPA handshakes found.\n" );
		return( 0 );
	}

	if( memcmp( ap_cur->essid, ZERO, 32 ) == 0 && ! opt.essid_set )
	{
		printf( "An ESSID is required. Try option -e.\n" );
		return( 0 );
	}

	if( opt.essid_set && ap_cur->essid[0] == '\0' )
	{
		memset(  ap_cur->essid, 0, sizeof( ap_cur->essid ) );
		strncpy( ap_cur->essid, opt.essid, sizeof( ap_cur->essid ) - 1 );
	}

	printf("\n\nBuilding Hashcat (1.00) file...\n\n");

	printf("[*] ESSID (length: %d): %s\n", (int)strlen(ap_cur->essid), ap_cur->essid);

	printf("[*] Key version: %d\n", ap_cur->wpa.keyver);

	printf("[*] BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
		ap_cur->bssid[0], ap_cur->bssid[1],
		ap_cur->bssid[2], ap_cur->bssid[3],
		ap_cur->bssid[4], ap_cur->bssid[5]
		);
	printf("[*] STA: %02X:%02X:%02X:%02X:%02X:%02X",
		ap_cur->wpa.stmac[0], ap_cur->wpa.stmac[1],
		ap_cur->wpa.stmac[2], ap_cur->wpa.stmac[3],
		ap_cur->wpa.stmac[4], ap_cur->wpa.stmac[5]
		);

	printf("\n[*] anonce:");
	for(i = 0; i < 32; i++)
	{
		if(i % 16 == 0) printf("\n    ");
		printf("%02X ", ap_cur->wpa.anonce[i]);
	}

	printf("\n[*] snonce:");
	for(i = 0; i < 32; i++)
	{
		if(i % 16 == 0) printf("\n    ");
		printf("%02X ", ap_cur->wpa.snonce[i]);
	}

	printf("\n[*] Key MIC:\n   ");
	for(i = 0; i < 16; i++)
	{
		printf(" %02X", ap_cur->wpa.keymic[i]);
	}

	printf("\n[*] hsPackets:");
	for( i = 0; i < ap_cur->wpa.eapol_size; i++)
	{
		if( i % 16 == 0 ) printf("\n    ");
		printf("%02X ",ap_cur->wpa.eapol[i]);

	}

	printf("\n");

	// write file
	FILE * fp_hccap;

	strcat(opt.hccap, ".hccap");

	fp_hccap = fopen( opt.hccap,"wb" );
	if (fp_hccap == NULL)
	{
		printf("\nFailed to create Hashcat capture file\n");
		return 0;
	}

	typedef struct
	{
		char          essid[36];

		unsigned char mac1[6];
		unsigned char mac2[6];
		unsigned char nonce1[32];
		unsigned char nonce2[32];

		unsigned char eapol[256];
		int           eapol_size;

		int           keyver;
		unsigned char keymic[16];

	} hccap_t;

	hccap_t hccap;

	memset (&hccap, 0, sizeof (hccap));

	memcpy (&hccap.essid,      &ap_cur->essid,          sizeof (ap_cur->essid));
	memcpy (&hccap.mac1,       &ap_cur->bssid,          sizeof (ap_cur->bssid));
	memcpy (&hccap.mac2,       &ap_cur->wpa.stmac,      sizeof (ap_cur->wpa.stmac));
	memcpy (&hccap.nonce1,     &ap_cur->wpa.snonce,     sizeof (ap_cur->wpa.snonce));
	memcpy (&hccap.nonce2,     &ap_cur->wpa.anonce,     sizeof (ap_cur->wpa.anonce));
	memcpy (&hccap.eapol,      &ap_cur->wpa.eapol,      sizeof (ap_cur->wpa.eapol));
	memcpy (&hccap.eapol_size, &ap_cur->wpa.eapol_size, sizeof (ap_cur->wpa.eapol_size));
	memcpy (&hccap.keyver,     &ap_cur->wpa.keyver,     sizeof (ap_cur->wpa.keyver));
	memcpy (&hccap.keymic,     &ap_cur->wpa.keymic,     sizeof (ap_cur->wpa.keymic));

	elt_written = fwrite(&hccap, sizeof (hccap_t), 1, fp_hccap);
	fclose(fp_hccap);

	if ((int)elt_written == 1) {
		printf("\nSuccessfully written to %s\n", opt.hccap);
	} else {
		printf("\nFailed to write to %s\n !", opt.hccap);
	}

	return( 1 );
}



/*
Uses the PTW attack to crack the WEP key.

Return: SUCCESS if it cracked the key,
        FAILURE if it could not.
*/
static int crack_wep_ptw(struct AP_info *ap_cur)
{
    int (* all)[256];
    int i, j, len = 0;

    opt.ap = ap_cur;

    all = malloc(32*sizeof(int [256]));
    if (all == NULL) {
    	return FAILURE;
    }

    //initial setup (complete keyspace)
    for (i = 0; i < 32; i++) {
        for (j = 0; j < 256; j++) {
            all[i][j] = 1;
        }
    }

    //setting restricted keyspace
    for (i = 0; i < 32; i++) {
        for (j = 0; j < 256; j++) {
            if( (opt.is_alnum && (j<32 || j>=128) ) ||
                (opt.is_fritz && (j<48 || j>=58)) ||
                (opt.is_bcdonly && ( j > 0x99 || ( j & 0x0F ) > 0x09 )) )
                all[i][j] = 0;
        }
    }

    //if debug is specified, force a specific value.
    for (i=0; i<32; i++) {
        for (j = 0; j < 256; j++) {
            if(opt.debug_row[i] == 1 && opt.debug[i] != j)
                all[i][j] = 0;
            else if(opt.debug_row[i] == 1 && opt.debug[i] == j)
                all[i][j] = 1;
        }
    }

    if(ap_cur->nb_ivs_clean > 99)
    {
        ap_cur->nb_ivs = ap_cur->nb_ivs_clean;
        //first try without bruteforcing, using only "clean" keystreams
        if(opt.keylen != 13)
        {
            if(PTW_computeKey(ap_cur->ptw_clean, wep.key, opt.keylen, (KEYLIMIT*opt.ffact), PTW_DEFAULTBF, all, opt.ptw_attack) == 1)
                len = opt.keylen;
        }
        else
        {
            /* try 1000 40bit keys first, to find the key "instantly" and you don't need to wait for 104bit to fail */
            if(PTW_computeKey(ap_cur->ptw_clean, wep.key, 5, 1000, PTW_DEFAULTBF, all, opt.ptw_attack) == 1)
                len = 5;
            else if(PTW_computeKey(ap_cur->ptw_clean, wep.key, 13, (KEYLIMIT*opt.ffact), PTW_DEFAULTBF, all, opt.ptw_attack) == 1)
                len = 13;
            else if(PTW_computeKey(ap_cur->ptw_clean, wep.key, 5, (KEYLIMIT*opt.ffact)/3, PTW_DEFAULTBF, all, opt.ptw_attack) == 1)
                len = 5;
        }
    }
    if(!len)
    {
        ap_cur->nb_ivs = ap_cur->nb_ivs_vague;
        //in case its not found, try bruteforcing the id field and include "vague" keystreams
        PTW_DEFAULTBF[10]=1;
        PTW_DEFAULTBF[11]=1;
//        PTW_DEFAULTBF[12]=1;

        if(opt.keylen != 13)
        {
            if(PTW_computeKey(ap_cur->ptw_vague, wep.key, opt.keylen, (KEYLIMIT*opt.ffact), PTW_DEFAULTBF, all, opt.ptw_attack) == 1)
                len = opt.keylen;
        }
        else
        {
            /* try 1000 40bit keys first, to find the key "instantly" and you don't need to wait for 104bit to fail */
            if(PTW_computeKey(ap_cur->ptw_vague, wep.key, 5, 1000, PTW_DEFAULTBF, all, opt.ptw_attack) == 1)
                len = 5;
            else if(PTW_computeKey(ap_cur->ptw_vague, wep.key, 13, (KEYLIMIT*opt.ffact), PTW_DEFAULTBF, all, opt.ptw_attack) == 1)
                len = 13;
            else if(PTW_computeKey(ap_cur->ptw_vague, wep.key, 5, (KEYLIMIT*opt.ffact)/10, PTW_DEFAULTBF, all, opt.ptw_attack) == 1)
                len = 5;
        }
    }

    if (!len)
            return FAILURE;

    opt.probability = 100;
    key_found(wep.key, len, -1);

    return SUCCESS;
}

void stop_attack_crack_wep(int stop_attack) {
    stop_att = stop_attack;
    close_aircrack = 1;
}


int attack_crack_wep(JNIEnv* pEnv, jobject pThis, jmethodID pMid, char* my_bssid, int use_essid, char* my_essid, char* file, int use_decloak, int use_korek, int my_id)
{
	int i, n, ret, option, j, ret1, nbMergeBSSID, unused;
	int cpu_count, showhelp, z, zz, forceptw;
	char *s, buf[128];
	struct AP_info *ap_cur;
	int old=0;
	char essid[33];

	buffer = NULL;			/* from read_thread */

	nb_eof = 0;				 /* # of threads who reached eof */
	nb_pkt = 0;				 /* # of packets read so far     */

	wepkey_crack_success = 0;
	close_aircrack = 0;
	id=0;

	wpa_wordlists_done = 0;

	(*pEnv)->GetJavaVM(pEnv, &jvm);  

	cb_jnienv = pEnv;
    cb_obj = (*pEnv)->NewGlobalRef(pEnv, pThis);
    cb_mid = pMid;
    cb_id = my_id;

#ifdef HAVE_SQLITE
	int rc;
	char *zErrMsg = 0;
	char looper[4] = {'|','/','-','\\'};
	int looperc = 0;
	int waited = 0;
	char *sqlformat = "SELECT pmk.PMK, passwd.passwd FROM pmk INNER JOIN passwd ON passwd.passwd_id = pmk.passwd_id INNER JOIN essid ON essid.essid_id = pmk.essid_id WHERE essid.essid = '%q'";
	char *sql;
#endif

#ifdef USE_GCRYPT
	// Register callback functions to ensure proper locking in the sensitive parts of libgcrypt < 1.6.0
	#if GCRYPT_VERSION_NUMBER < 0x010600
		gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
	#endif
	// Disable secure memory.
	gcry_control (GCRYCTL_DISABLE_SECMEM, 0);
	// Tell Libgcrypt that initialization has completed.
	gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
#endif
	ret = FAILURE;
	showhelp = 0;
	stop_att = 0;

	start_logger("aircrack-ng");
	// Start a new process group, we are perhaps going to call kill(0, ...) later
	setsid();

	progname = getVersion("Aircrack-ng", _MAJ, _MIN, _SUB_MIN, _REVISION, _BETA, _RC);

	memset( &opt, 0, sizeof( opt ) );

	srand( time( NULL ) );
	if(stop_att)
		goto exit_main;
	// Get number of CPU (return -1 if failed).
	cpu_count = get_nb_cpus();
	opt.nbcpu = 1;
	if (cpu_count > 1) {
		opt.nbcpu = cpu_count;
	}

	j=0;
	/* check the arguments */

	opt.nbdict		= 0;
	opt.amode		= 0;
	opt.do_brute    = 1;
	opt.do_mt_brute = 1;
	opt.showASCII   = 0;
	opt.probability = 51;
	opt.next_ptw_try= 0;
	opt.do_ptw		= 1;
	opt.max_ivs		= INT_MAX;
	opt.visual_inspection = 0;
	opt.firstbssid	= NULL;
	opt.bssid_list_1st = NULL;
	opt.bssidmerge	= NULL;
	opt.oneshot		= 0;
	opt.logKeyToFile = NULL;
	opt.wkp = NULL;
	opt.hccap = NULL;
	opt.forced_amode	= 0;
	opt.is_quiet = 1;
	opt.oneshot = 1;

	/*
	all_ivs = malloc( (256*256*256) * sizeof(used_iv));
	memset(all_ivs, 0, (256*256*256)*sizeof(used_iv));
	*/

	if(use_korek)
		opt.do_ptw = 0;

	forceptw = 0;

	if(use_essid) {
		memset(opt.essid, 0, sizeof( opt.essid ) );
				strncpy( opt.essid, my_essid, sizeof( opt.essid ) - 1 );
				opt.essid_set = 1;
	}

	getmac(my_bssid, 1, opt.bssid);
	opt.bssid_set = 1;

	if(use_decloak)
		opt.wep_decloak = 1;

	if (_speed_test) {
		opt.amode = 2;
		opt.dict = stdin;
		opt.bssid_set = 1;

		ap_1st = ap_cur = malloc(sizeof(*ap_cur));
		if (!ap_cur)
			err(1, "malloc()");

		memset(ap_cur, 0, sizeof(*ap_cur));

		ap_cur->target = 1;
		ap_cur->wpa.state = 7;
		strcpy(ap_cur->essid, "sorbo");

		goto __start;
	}

	
	if( opt.amode == 2 && opt.dict == NULL )
	{
		nodict:
		if (opt.wkp == NULL && opt.hccap == NULL)
		{
			printf( "Please specify a dictionary (option -w).\n" );
		}
		else
		{
			if(stop_att)
				goto exit_main;
			if (opt.wkp)
			{
				ap_cur = ap_1st;
				ret = do_make_wkp(ap_cur);
			}
			if (opt.hccap)
			{
				ap_cur = ap_1st;
				ret = do_make_hccap(ap_cur);
			}
	}
	goto exit_main;
	}

	if( (! opt.essid_set && ! opt.bssid_set) && ( opt.is_quiet || opt.no_stdin ) )
	{
		printf( "Please specify an ESSID or BSSID.\n" );
		snprintf(info_buffer, sizeof(info_buffer), "Please specify an ESSID or BSSID.\n");
                	info_message(info_buffer, UPDATE_ERROR);
		goto exit_main;
	}

	/* start one thread per input file */

	signal( SIGINT,  sighandler );
	signal( SIGQUIT, sighandler );
	signal( SIGTERM, sighandler );
	signal( SIGALRM, SIG_IGN );



	pthread_mutex_init( &mx_apl, NULL );
	pthread_mutex_init( &mx_ivb, NULL );
	pthread_mutex_init( &mx_eof, NULL );
	pthread_mutex_init( &mx_dic, NULL );
	pthread_cond_init(  &cv_eof, NULL );

	ap_1st = NULL;

	id = 0;

	

	nb_eof=0;
	signal( SIGINT, sighandler );

	
		if( strcmp( file, "-" ) == 0 )
			opt.no_stdin = 1;
		if(stop_att)
			goto exit_main;
		if( pthread_create( &(tid[id]), NULL, (void *) read_thread,
			(void *) file ) != 0 )
		{
			perror( "pthread_create failed" );
			goto exit_main;
		}

		id++;
		usleep( 131071 );
		
	
	nb_pkt=0;

	/* wait until each thread reaches EOF */

	intr_read=0;
	pthread_mutex_lock( &mx_eof );

	if(stop_att)
		goto exit_main;
		printf( "Reading packets, please wait...\r" );
		snprintf(info_buffer, sizeof(info_buffer), "Reading packets, please wait...\n");
                	info_message(info_buffer, UPDATE_RUNNING);
		fflush( stdout );
	

	while( nb_eof < n && ! intr_read ) {
		if(stop_att)
			goto exit_main;
		
		pthread_cond_wait( &cv_eof, &mx_eof );

	}
	

	pthread_mutex_unlock( &mx_eof );

	intr_read=1;
// 	if( ! opt.is_quiet && ! opt.no_stdin )
// 		printf( "\33[KRead %ld packets.\n\n", nb_pkt );

// 	#ifndef DO_PGO_DUMP
// 	signal( SIGINT, SIG_DFL );	 /* we want sigint to stop and dump pgo data */
// 	#endif

	/* mark the targeted access point(s) */

	ap_cur = ap_1st;

	while( ap_cur != NULL )
	{
		if(stop_att)
			goto exit_main;
		if( memcmp( opt.maddr, BROADCAST, 6 ) == 0 ||
			( opt.bssid_set && ! memcmp( opt.bssid, ap_cur->bssid, 6 ) ) ||
			( opt.essid_set && ! strcmp( opt.essid, ap_cur->essid    ) ) )
			ap_cur->target = 1;

		ap_cur = ap_cur->next;
	}

	ap_cur = ap_1st;

	while( ap_cur != NULL )
	{
		if(stop_att)
			goto exit_main;
		if( ap_cur->target )
			break;

		ap_cur = ap_cur->next;
	}

	if( ap_cur == NULL )
	{
		printf( "No matching network found - check your %s.\n",
			( opt.essid_set ) ? "essid" : "bssid" );
		snprintf(info_buffer, sizeof(info_buffer), "No matching network found - check your %s.\n",
			( opt.essid_set ) ? "essid" : "bssid");
                	info_message(info_buffer, UPDATE_ERROR);
		goto exit_main;
	}

	if( ap_cur->crypt < 2 )
	{
		switch( ap_cur->crypt )
		{
			case  0:
				printf( "Target network doesn't seem encrypted.\n" );
				snprintf(info_buffer, sizeof(info_buffer), "Target network doesn't seem encrypted.\n");
                	info_message(info_buffer, UPDATE_ERROR);
				break;

			default:
				printf( "Got no data packets from target network!\n" );
				snprintf(info_buffer, sizeof(info_buffer), "Got no data packets from target network!\n");
                	info_message(info_buffer, UPDATE_ERROR);
				break;
		}

		goto exit_main;
	}

	/* create the cracker<->master communication pipes */

	for( i = 0; i < opt.nbcpu; i++ )
	{
		unused = pipe( mc_pipe[i] );
		unused = pipe( cm_pipe[i] );
		if(stop_att)
			goto exit_main;
		if (opt.amode<=1 && opt.nbcpu>1 && opt.do_brute && opt.do_mt_brute)
		{
			unused = pipe(bf_pipe[i]);
			bf_nkeys[i] = 0;
		}
	}

__start:
	/* launch the attack */

	pthread_mutex_lock(&mx_nb);
	nb_tried = 0;
	nb_kprev = 0;
	pthread_mutex_unlock(&mx_nb);

	chrono( &t_begin, 1 );
	chrono( &t_stats, 1 );
	chrono( &t_kprev, 1 );

	signal( SIGWINCH, sighandler );

	
		

	/* CRACK WEP, Default key length: 128 bits */
	if( opt.keylen == 0 )
		opt.keylen = 13;

	if(j + opt.do_brute > 4)
	{
		printf( "Bruteforcing more than 4 bytes will take too long, aborting!" );
		snprintf(info_buffer, sizeof(info_buffer), "Bruteforcing more than 4 bytes will take too long, aborting!");
                	info_message(info_buffer, UPDATE_ERROR);
		goto exit_main;
	}

	for( i=0; i<opt.do_brute; i++)
	{
		opt.brutebytes[j+i] = opt.keylen -1 -i;
		if(stop_att)
			goto exit_main;
	}

	opt.do_brute += j;

	if( opt.ffact == 0 )
	{
                    if( opt.do_ptw ) opt.ffact = 2;
                    else
                    {
                        if( ! opt.do_testy )
                        {
                            if( opt.keylen == 5 )
                                opt.ffact = 5;
			else
                                opt.ffact = 2;
                        }
                        else
                            opt.ffact = 30;
                    }
	}

	memset( &wep, 0, sizeof( wep ) );
	if(stop_att)
		goto exit_main;
	if (opt.do_ptw)
	{
		
		opt.next_ptw_try = ap_cur->nb_ivs_vague - (ap_cur->nb_ivs_vague % PTW_TRY_STEP);
		do
		{
			if(ap_cur->nb_ivs_vague >= opt.next_ptw_try)
			{
					if(stop_att)
						goto exit_main;
					printf("Starting PTW attack with %ld ivs.\n", ap_cur->nb_ivs_vague);
					snprintf(info_buffer, sizeof(info_buffer), "Starting PTW attack.\n");
                	info_message(info_buffer, UPDATE_RUNNING);
				
				ret = crack_wep_ptw(ap_cur);

				if( opt.oneshot == 1 && ret == FAILURE )
				{
					printf( "   Attack failed. Possible reasons:\n\n"
						"     * Out of luck: you must capture more IVs. Usually, 104-bit WEP\n"
						"       can be cracked with about 80 000 IVs, sometimes more.\n\n"
						"     * Try to raise the fudge factor (-f).\n");

					snprintf(info_buffer, sizeof(info_buffer), "   Attack failed. Possible reasons:\n\n"
						"     * Out of luck: you must capture more IVs. Usually, 104-bit WEP\n"
						"       can be cracked with about 80 000 IVs, sometimes more.\n\n"
						"     * Try to raise the fudge factor.\n");
                	info_message(info_buffer, UPDATE_ERROR);

					ret=0;
				}

				if(ret)
				{
					opt.next_ptw_try += PTW_TRY_STEP;
					printf("Failed. Next try with %d IVs.\n", opt.next_ptw_try);
					snprintf(info_buffer, sizeof(info_buffer), "Failed: Not enough IVs.\n", opt.next_ptw_try);
                	info_message(info_buffer, UPDATE_ERROR);
                	ret = 0;
				}
			}
			if(ret)
				usleep(10000);
		}while(ret != 0);
	}
	
	else
	{
	
		for( i = 0; i < opt.nbcpu; i++ )
		{
			/* start one thread per cpu */

			if (opt.amode<=1 && opt.nbcpu>1 && opt.do_brute && opt.do_mt_brute)
			{
				if (pthread_create( &(tid[id]), NULL, (void *) inner_bruteforcer_thread,
					(void *) (long) i ) != 0)
				{
					perror( "pthread_create failed" );
					goto exit_main;
				}
				id++;
			}

			if( pthread_create( &(tid[id]), NULL, (void *) crack_wep_thread,
				(void *) (long) i ) != 0 )
			{
				perror( "pthread_create failed" );
				goto exit_main;
			}
			id++;
		}
		
		if( ! opt.do_testy )
		{
			do   { 

					ret = do_wep_crack1( 0 ); 
					
					if(stop_att)
						goto exit_main; 
			} while( ret == RESTART );

			if( ret == FAILURE )
			{
				printf( "   Attack failed. Possible reasons:\n\n"
					"     * Out of luck: you must capture more IVs. Usually, 104-bit WEP\n"
					"       can be cracked with about one million IVs, sometimes more.\n\n"
					"     * If all votes seem equal, or if there are many negative votes,\n"
					"       then the capture file is corrupted, or the key is not static.\n\n"
					"     * A false positive prevented the key from being found.  Try to\n"
					"       disable each korek attack (-k 1 .. 17), raise the fudge factor\n"
					"       (-f)" );
				snprintf(info_buffer, sizeof(info_buffer), "   Attack failed. Possible reasons:\n\n"
					"     * Out of luck: you must capture more IVs. Usually, 104-bit WEP\n"
					"       can be cracked with about one million IVs, sometimes more.\n\n"
					"     * If all votes seem equal, or if there are many negative votes,\n"
					"       then the capture file is corrupted, or the key is not static.\n\n"
					"     * A false positive prevented the key from being found.  Try to\n"
					"       disable each korek attack (-k 1 .. 17), raise the fudge factor\n" );
                info_message(info_buffer, UPDATE_ERROR);
				if (opt.do_testy)
					printf( "and try the experimental bruteforce attacks (-y)." );
				printf( "\n" );
				if(stop_att)
					goto exit_main;
			}
		}
		else
		{
			for( i = opt.keylen - 3; i < opt.keylen - 2; i++ )
			{
				do   { 
					
					ret = do_wep_crack2( i );
						if(stop_att)
							goto exit_main; 
				} while( ret == RESTART );

				if( ret == SUCCESS )
					break;
			}

			if( ret == FAILURE )
			{
				printf( "   Attack failed. Possible reasons:\n\n"
					"     * Out of luck: you must capture more IVs. Usually, 104-bit WEP\n"
					"       can be cracked with about one million IVs, sometimes more.\n\n"
					"     * If all votes seem equal, or if there are many negative votes,\n"
					"       then the capture file is corrupted, or the key is not static.\n\n"
					"     * A false positive prevented the key from being found.  Try to\n"
					"       disable each korek attack (-k 1 .. 17), raise the fudge factor\n"
					"       (-f)" );
				snprintf(info_buffer, sizeof(info_buffer), "   Attack failed. Possible reasons:\n\n"
					"     * Out of luck: you must capture more IVs. Usually, 104-bit WEP\n"
					"       can be cracked with about one million IVs, sometimes more.\n\n"
					"     * If all votes seem equal, or if there are many negative votes,\n"
					"       then the capture file is corrupted, or the key is not static.\n\n"
					"     * A false positive prevented the key from being found.  Try to\n"
					"       disable each korek attack (-k 1 .. 17), raise the fudge factor\n" );
                info_message(info_buffer, UPDATE_ERROR);
				if (opt.do_testy)
					printf( "or try the standard attack mode instead (no -y option)." );
				printf( "\n" );
			}
		}
		
	}

	

		

	exit_main:

	#if ((defined(__INTEL_COMPILER) || defined(__ICC)) && defined(DO_PGO_DUMP))
	_PGOPTI_Prof_Dump();
	#endif
	if( ! opt.is_quiet )
		printf( "\n" );

	fflush( stdout );

// 	if( ret == SUCCESS ) kill( 0, SIGQUIT );
// 	if( ret == FAILURE ) kill( 0, SIGTERM );
	clean_exit(ret);
	(*pEnv)->DeleteGlobalRef(pEnv, cb_obj);

	return( ret );
}
