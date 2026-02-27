/*
 * coWPAtty - Brute-force dictionary attack against WPA-PSK.
 *
 * Copyright (c) 2004-2009, Joshua Wright <jwright@hasborg.com>
 *
 * $Id: cowpatty.c 264 2009-07-03 15:15:50Z jwright $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. See COPYING for more
 * details.
 *
 * coWPAtty is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Significant code is graciously taken from the following:
 * wpa_supplicant by Jouni Malinen.  This tool would have been MUCH more
 * difficult for me if not for this code.  Thanks Jouni.
 */

/*
 * Right off the bat, this code isn't very useful.  The PBKDF2 function makes
 * 4096 SHA-1 passes for each passphrase, which takes quite a bit of time.  On
 * my Pentium II development system, I'm getting ~2.5 passphrases/second.
 * I've done my best to optimize the PBKDF2 function, but it's still pretty
 * slow.
 */

#define PROGNAME "cowpatty"
#define VER "4.6"
#define MAXPASSPHRASE 256
#define DOT1X_LLCTYPE "\x88\x8e"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pcap.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <jni.h>

#include "cowpatty.h"
#include "common.h"
#include "utils.h"
#include "sha1.h"
#include "md5.h"
#include "radiotap.h"
#include "infomessage.h"

/* Globals */
pcap_t *p = NULL;
unsigned char *packet;
struct pcap_pkthdr *h;
char errbuf[PCAP_ERRBUF_SIZE];
int sig = 0;			/* Used for handling signals */
char *words;
/* temporary storage for the password from the hash record, instead of
   malloc/free for each entry. */
char password_buf[65];
unsigned long wordstested = 0;
int stop_attack_cowp = 0;
char mybuffer[1024];

/* Prototypes */
void wpa_pmk_to_ptk(u8 * pmk, u8 * addr1, u8 * addr2,
		    u8 * nonce1, u8 * nonce2, u8 * ptk, size_t ptk_len);
void hexdump(unsigned char *data, int len);
void usage(char *message);
int testopts(struct user_opt *opt);
void cleanup();
//void parseopts(struct user_opt *opt, int argc, char **argv);
void closepcap(struct capture_data *capdata);
void handle_dot1x(struct crack_data *cdata, struct capture_data *capdata,
		  struct user_opt *opt);
void dump_all_fields(struct crack_data cdata, struct user_opt *opt);
void printstats(struct timeval start, struct timeval end,
		unsigned long int wordcount);
int nextdictword(char *word, FILE * fp);
int nexthashrec(FILE * fp, struct hashdb_rec *rec);

void usage(char *message)
{

	if (strlen(message) > 0) {
		printf("%s: %s\n", PROGNAME, message);
	}

	printf("\nUsage: %s [options]\n", PROGNAME);
	printf("\n"
	       "\t-f \tDictionary file\n"
	       "\t-d \tHash file (genpmk)\n"
	       "\t-r \tPacket capture file\n"
	       "\t-s \tNetwork SSID (enclose in quotes if SSID includes spaces)\n"
	       "\t-2 \tUse frames 1 and 2 or 2 and 3 for key attack (nonstrict mode)\n"
           "\t-c \tCheck for valid 4-way frames, does not crack\n"
	       "\t-h \tPrint this help information and exit\n"
	       "\t-v \tPrint verbose information (more -v for more verbosity)\n"
	       "\t-V \tPrint program version and exit\n" "\n");
}

void cleanup()
{
	/* lame-o-meter++ */
	sig = 1;
}

void wpa_pmk_to_ptk(u8 * pmk, u8 * addr1, u8 * addr2,
		    u8 * nonce1, u8 * nonce2, u8 * ptk, size_t ptk_len)
{
	u8 data[2 * ETH_ALEN + 2 * 32];

	memset(&data, 0, sizeof(data));

	/* PTK = PRF-X(PMK, "Pairwise key expansion",
	 *             Min(AA, SA) || Max(AA, SA) ||
	 *             Min(ANonce, SNonce) || Max(ANonce, SNonce)) */

	if (memcmp(addr1, addr2, ETH_ALEN) < 0) {
		memcpy(data, addr1, ETH_ALEN);
		memcpy(data + ETH_ALEN, addr2, ETH_ALEN);
	} else {
		memcpy(data, addr2, ETH_ALEN);
		memcpy(data + ETH_ALEN, addr1, ETH_ALEN);
	}

	if (memcmp(nonce1, nonce2, 32) < 0) {
		memcpy(data + 2 * ETH_ALEN, nonce1, 32);
		memcpy(data + 2 * ETH_ALEN + 32, nonce2, 32);
	} else {
		memcpy(data + 2 * ETH_ALEN, nonce2, 32);
		memcpy(data + 2 * ETH_ALEN + 32, nonce1, 32);
	}

	sha1_prf(pmk, 32, "Pairwise key expansion", data, sizeof(data),
		 ptk, ptk_len);
}

void hexdump(unsigned char *data, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		printf("%02x ", data[i]);
	}
}


/*
void parseopts(struct user_opt *opt, int argc, char **argv)
{

	int c;

	while ((c = getopt(argc, argv, "f:r:s:d:c2nhvV")) != EOF) {
		switch (c) {
		case 'f':
			strncpy(opt->dictfile, optarg, sizeof(opt->dictfile));
			break;
		case 'r':
			strncpy(opt->pcapfile, optarg, sizeof(opt->pcapfile));
			break;
		case 's':
			strncpy(opt->ssid, optarg, sizeof(opt->ssid));
			break;
		case 'd':
			strncpy(opt->hashfile, optarg, sizeof(opt->hashfile));
			break;
		case 'n':
		case '2':
			opt->nonstrict++;
			break;
		case 'c':
			opt->checkonly++;
			break;
		case 'h':
			usage("");
			exit(0);
			break;
		case 'v':
			opt->verbose++;
			break;
		case 'V':
			printf
				("$Id: cowpatty.c 264 2009-07-03 15:15:50Z jwright $\n");
			exit(0);
			break;
		default:
			usage("");
			exit(-1);
		}
	}
}
*/
int testopts(struct user_opt *opt)
{
	struct stat teststat;

    /* Test for a pcap file */
	if (IsBlank(opt->pcapfile)) {
		usage("Must supply a pcap file with -r");

		snprintf(mybuffer, sizeof(mybuffer), "Must supply a pcap file!\n");
        info_message(mybuffer, UPDATE_ERROR);
		return(-1);
	}

    if (opt->checkonly == 1) {
        /* Special case where we only need pcap file */
        return 1;
    }

	/* test for required parameters */
	if (IsBlank(opt->dictfile) && IsBlank(opt->hashfile)) {
		usage("Must supply a list of passphrases in a file with -f "
		      "or a hash file\n\t  with -d.  "
		      "Use \"-f -\" to accept words on stdin.");
		return(-1);
	}

	if (IsBlank(opt->ssid)) {
		usage("Must supply the SSID for the network with -s");

		snprintf(mybuffer, sizeof(mybuffer), "Must supply the SSID for the network!\n");
        info_message(mybuffer, UPDATE_ERROR);
		return(-1);
	}

	/* Test that the files specified exist and are greater than 0 bytes */
	if (!IsBlank(opt->hashfile) && strncmp(opt->hashfile, "-", 1) != 0) {
		if (stat(opt->hashfile, &teststat)) {
			usage("Could not stat hashfile.  Check file path.");

			snprintf(mybuffer, sizeof(mybuffer), "Could not stat hashfile. Check file path.\n");
        info_message(mybuffer, UPDATE_ERROR);
			return(-1);
		} else if (teststat.st_size == 0) {
			usage("Empty hashfile (0 bytes).  Check file contents.");

			snprintf(mybuffer, sizeof(mybuffer), "Empty hashfile (0 bytes). Check file contents.\n");
        info_message(mybuffer, UPDATE_ERROR);
			return(-1);
		}
	}

	if (!IsBlank(opt->dictfile) && strncmp(opt->dictfile, "-", 1) != 0) {
		if (stat(opt->dictfile, &teststat)) {
			usage
			    ("Could not stat the dictionary file.  Check file path.");

			    snprintf(mybuffer, sizeof(mybuffer), "Could not stat the dictionary file. Check file path.\n");
        info_message(mybuffer, UPDATE_ERROR);
			return(-1);
		} else if (teststat.st_size == 0) {
			usage
			    ("Empty dictionary file (0 bytes).  Check file contents.");

			    snprintf(mybuffer, sizeof(mybuffer), "Empty dictionary file (0 bytes).  Check file contents.\n");
        info_message(mybuffer, UPDATE_ERROR);
			return(-1);
		}
	}

	if (stat(opt->pcapfile, &teststat) && strncmp(opt->hashfile, "-", 1) != 0) {
		usage("Could not stat the pcap file.  Check file path.");

		snprintf(mybuffer, sizeof(mybuffer), "Could not stat the pcap file. Check file path.\n");
        info_message(mybuffer, UPDATE_ERROR);

		return(-1);
	} else if (teststat.st_size == 0) {
		usage("Empty pcap file (0 bytes).  Check file contents.");

		snprintf(mybuffer, sizeof(mybuffer), "Empty pcap file (0 bytes). Check file contents.\n");
        info_message(mybuffer, UPDATE_ERROR);

		return(-1);
	}
}

int openpcap(struct capture_data *capdata)
{

	/* Assume for now it's a libpcap file */
	p = pcap_open_offline(capdata->pcapfilename, errbuf);
	if (p == NULL) {
		perror("Unable to open capture file");
		snprintf(mybuffer, sizeof(mybuffer), "Unable to open capture file!\n");
        info_message(mybuffer, UPDATE_ERROR);
		return (-1);
	}

	/* Determine link type */
	capdata->pcaptype = pcap_datalink(p);

	/* Determine offset to EAP frame based on link type */
	switch (capdata->pcaptype) {
	case DLT_NULL:
	case DLT_EN10MB:
	case DLT_IEEE802_11:
	case DLT_PRISM_HEADER:
	case DLT_IEEE802_11_RADIO:
		break;
	default:
		/* Unknown/unsupported pcap type */
		snprintf(mybuffer, sizeof(mybuffer), "Unknown / unsupported pcap type!\n");
        info_message(mybuffer, UPDATE_ERROR);	
		return (1);
	}

	return (0);
}

void closepcap(struct capture_data *capdata)
{

	/* Assume it's a libpcap file for now */
	pcap_close(p);
}

/* Populates global *packet, returns status */
int getpacket(struct capture_data *capdata)
{
	/* Assume it's a libpcap file for now */
	int ret;
	struct ieee80211_radiotap_header *rtaphdr;
	int rtaphdrlen=0;
	struct dot11hdr *dot11 = NULL;

	/* Loop on pcap_next_ex until we get
 a packet we want, return from
	 * this while loop.  This is kinda hack.
	 */
	while ((ret = pcap_next_ex(p, &h, (const u_char **)&packet)) 
			&& ret > 0) {
		/* Determine offset to EAP frame based on link type */
		switch (capdata->pcaptype) {
		case DLT_NULL:
		case DLT_EN10MB:
            
			/* Standard ethernet header */
			capdata->dot1x_offset = 14;
			capdata->l2type_offset = 12;
			capdata->dstmac_offset = 0;
			capdata->srcmac_offset = 6;
			return(ret);
			break;

		case DLT_IEEE802_11:
            
			/* If this is not a data packet, get the next frame */
			dot11 = (struct dot11hdr *)packet;
			if (dot11->u1.fc.type != DOT11_FC_TYPE_DATA) {
				continue;
			}

			capdata->dstmac_offset = 4;
			capdata->srcmac_offset = 10;

			/* differentiate QoS data and non-QoS data frames */
			if (dot11->u1.fc.subtype == DOT11_FC_SUBTYPE_QOSDATA) {
				/* 26 bytes 802.11 header, 8 for 802.2 header */
		
				capdata->dot1x_offset = 34;
				capdata->l2type_offset = 32;
			} else if (dot11->u1.fc.subtype == DOT11_FC_SUBTYPE_DATA) {
				/* 24 bytes 802.11 header, 8 for 802.2 header */
			
				capdata->dot1x_offset = 32;
				capdata->l2type_offset = 30;
			} else {
				/* Not a data frame we support */
				continue;
			}
			return(ret);
			break;

		case DLT_PRISM_HEADER:
			/* 802.11 frames with AVS header, AVS header is 144
			 * bytes */
		
			/* If this is not a data packet, get the next frame */
			dot11 = ((struct dot11hdr *)(packet+144));
			if (dot11->u1.fc.type != DOT11_FC_TYPE_DATA) {
				continue;
			}
			capdata->dstmac_offset = 4 + 144;
			capdata->srcmac_offset = 10 + 144;

			/* differentiate QoS data and non-QoS data frames */
			if (dot11->u1.fc.subtype == DOT11_FC_SUBTYPE_QOSDATA) {
				capdata->dot1x_offset = 34 + 144;
				capdata->l2type_offset = 32 + 144;
			} else if (dot11->u1.fc.subtype == DOT11_FC_SUBTYPE_DATA) {
				capdata->dot1x_offset = 32 + 144;
				capdata->l2type_offset = 30 + 144;
			} else {
				/* Not a data frame we support */
				continue;
			}
			return(ret);
			break;

		case DLT_IEEE802_11_RADIO:
			/* Radiotap header, need to calculate offset to payload
			   on a per-packet basis.
			*/
			rtaphdr = (struct ieee80211_radiotap_header *)packet;
			/* rtap is LE */
			rtaphdrlen = le16_to_cpu(rtaphdr->it_len); 
	
			/* Sanity check on header length, 10 bytes is min 
			   802.11 len */
			if (rtaphdrlen > (h->len - 10)) {
				return -2; /* Bad radiotap data */
			}
	
			capdata->dstmac_offset = 4 + rtaphdrlen;
			capdata->srcmac_offset = 10 + rtaphdrlen;

			dot11 = ((struct dot11hdr *)(packet+rtaphdrlen));
			/* differentiate QoS data and non-QoS data frames */
			if (dot11->u1.fc.subtype == DOT11_FC_SUBTYPE_QOSDATA) {
				capdata->dot1x_offset = 34 + rtaphdrlen;
				capdata->l2type_offset = 32 + rtaphdrlen;
			} else if (dot11->u1.fc.subtype ==
					DOT11_FC_SUBTYPE_DATA) {
				capdata->dot1x_offset = 32 + rtaphdrlen;
				capdata->l2type_offset = 30 + rtaphdrlen;
			} else {
				/* Not a data frame we support */
				continue;
			}
			return(ret);
			break;

		default:
			snprintf(mybuffer, sizeof(mybuffer), "Unknown / unsupported pcap type!\n");
        	info_message(mybuffer, UPDATE_ERROR);		
			/* Unknown/unsupported pcap type */
			return (1);
		}
	}

	return (ret);
}

void handle_dot1x(struct crack_data *cdata, struct capture_data *capdata,
		  struct user_opt *opt)
{
	struct ieee8021x *dot1xhdr;
	struct wpa_eapol_key *eapolkeyhdr;
	int eapolkeyoffset;
	int key_info, index;

	/* We're going after the last three frames in the 4-way handshake.
	   In the last frame of the TKIP exchange, the authenticator nonce is
	   omitted.  In cases where there is a unicast and a multicast key 
	   distributed, frame 4 will include the authenticator nonce.  In some
	   cases however, there is no multicast key distribution, so frame 4 has
	   no authenticator nonce.  For this reason, we need to capture information
	   from the 2nd, 3rd and 4th frames to accommodate cases where there is no
	   multicast key delivery.  Suckage.
	 */

	dot1xhdr = (struct ieee8021x *)&packet[capdata->dot1x_offset];
	eapolkeyoffset = capdata->dot1x_offset + sizeof(struct ieee8021x);
	eapolkeyhdr = (struct wpa_eapol_key *)&packet[eapolkeyoffset];

	/* Bitwise fields in the key_info field of the EAPOL-Key header */
	key_info = be_to_host16(eapolkeyhdr->key_info);
	cdata->ver = key_info & WPA_KEY_INFO_TYPE_MASK;
	index = key_info & WPA_KEY_INFO_KEY_INDEX_MASK;

	if (opt->nonstrict == 0) {

	        /* Check for EAPOL version 1, type EAPOL-Key */
        	if (dot1xhdr->version != 1 || dot1xhdr->type != 3) {
                	return;
        	}

	} else {

		/* Check for type EAPOL-Key */
		if (dot1xhdr->type != 3) {
			return;
		}

	}
	if (cdata->ver != WPA_KEY_INFO_TYPE_HMAC_MD5_RC4 &&
		cdata->ver != WPA_KEY_INFO_TYPE_HMAC_SHA1_AES) {
		return;
	}

	if (cdata->ver == WPA_KEY_INFO_TYPE_HMAC_MD5_RC4) {
		/* Check for WPA key, and pairwise key type */
		if (eapolkeyhdr->type != 254 || 
				(key_info & WPA_KEY_INFO_KEY_TYPE) == 0) {
			return;
		}
	} else if (cdata->ver == WPA_KEY_INFO_TYPE_HMAC_SHA1_AES) {
		if (eapolkeyhdr->type != 2 ||
				(key_info & WPA_KEY_INFO_KEY_TYPE) == 0) {
			return;
		}
	}

	if (opt->nonstrict == 0) {

		/* Check for frame 2 of the 4-way handshake */
		if ((key_info & WPA_KEY_INFO_MIC)
			&& (key_info & WPA_KEY_INFO_ACK) == 0
			&& (key_info & WPA_KEY_INFO_INSTALL) == 0
			&& eapolkeyhdr->key_data_length > 0) {

			/* All we need from this frame is the authenticator nonce */
			memcpy(cdata->snonce, eapolkeyhdr->key_nonce,
			       sizeof(cdata->snonce));
			cdata->snonceset = 1;

		/* Check for frame 3 of the 4-way handshake */
		} else if ((key_info & WPA_KEY_INFO_MIC)
			  && (key_info & WPA_KEY_INFO_INSTALL)
			  && (key_info & WPA_KEY_INFO_ACK)) {

			memcpy(cdata->spa, &packet[capdata->dstmac_offset],
			       sizeof(cdata->spa));
			memcpy(cdata->aa, &packet[capdata->srcmac_offset],
			       sizeof(cdata->aa));
			memcpy(cdata->anonce, eapolkeyhdr->key_nonce,
			       sizeof(cdata->anonce));
			cdata->aaset = 1;
			cdata->spaset = 1;
			cdata->anonceset = 1;
			/* We save the replay counter value in the 3rd frame to match
			   against the 4th frame of the four-way handshake */
			memcpy(cdata->replay_counter,
			       eapolkeyhdr->replay_counter, 8);

		/* Check for frame 4 of the four-way handshake */
		} else if ((key_info & WPA_KEY_INFO_MIC)
			  && (key_info & WPA_KEY_INFO_ACK) == 0
			  && (key_info & WPA_KEY_INFO_INSTALL) == 0
			  && (memcmp (cdata->replay_counter,
			      eapolkeyhdr->replay_counter, 8) == 0)) {

			memcpy(cdata->keymic, eapolkeyhdr->key_mic,
			       sizeof(cdata->keymic));
			memcpy(cdata->eapolframe, &packet[capdata->dot1x_offset],
			       sizeof(cdata->eapolframe));
			cdata->keymicset = 1;
			cdata->eapolframeset = 1;
		}
	} else {

		/* Check for frame 1 of the 4-way handshake */
		if ((key_info & WPA_KEY_INFO_MIC) == 0 
		   && (key_info & WPA_KEY_INFO_ACK)
		   && (key_info & WPA_KEY_INFO_INSTALL) == 0 ) {
	                /* All we need from this frame is the authenticator nonce */
			memcpy(cdata->anonce, eapolkeyhdr->key_nonce,
				sizeof(cdata->anonce));
			cdata->anonceset = 1;
 
		/* Check for frame 2 of the 4-way handshake */
		} else if ((key_info & WPA_KEY_INFO_MIC)
			  && (key_info & WPA_KEY_INFO_INSTALL) == 0
			  && (key_info & WPA_KEY_INFO_ACK) == 0
			  && eapolkeyhdr->key_data_length > 0) {

			cdata->eapolframe_size = ( packet[capdata->dot1x_offset + 2] << 8 )
					+   packet[capdata->dot1x_offset + 3] + 4;

			memcpy(cdata->spa, &packet[capdata->dstmac_offset],
				sizeof(cdata->spa));
			cdata->spaset = 1;

			memcpy(cdata->aa, &packet[capdata->srcmac_offset],
				sizeof(cdata->aa));
			cdata->aaset = 1;

			memcpy(cdata->snonce, eapolkeyhdr->key_nonce,
				 sizeof(cdata->snonce));
			cdata->snonceset = 1;

			memcpy(cdata->keymic, eapolkeyhdr->key_mic,
				sizeof(cdata->keymic));
			cdata->keymicset = 1;

			memcpy(cdata->eapolframe, &packet[capdata->dot1x_offset],
				cdata->eapolframe_size);
			cdata->eapolframeset = 1;


        /* Check for frame 3 of the 4-way handshake */
		}  else if ((key_info & WPA_KEY_INFO_MIC)
			  	&& (key_info & WPA_KEY_INFO_ACK)
	   			&& (key_info & WPA_KEY_INFO_INSTALL)) {
			/* All we need from this frame is the authenticator nonce */
			memcpy(cdata->anonce, eapolkeyhdr->key_nonce,
			sizeof(cdata->anonce));
			cdata->anonceset = 1;
		}
	}
}

void dump_all_fields(struct crack_data cdata, struct user_opt *opt)
{

	printf("AA is:");
	lamont_hdump(cdata.aa, 6);
	printf("\n");

	printf("SPA is:");
	lamont_hdump(cdata.spa, 6);
	printf("\n");

	printf("snonce is:");
	lamont_hdump(cdata.snonce, 32);
	printf("\n");

	printf("anonce is:");
	lamont_hdump(cdata.anonce, 32);
	printf("\n");

	printf("keymic is:");
	lamont_hdump(cdata.keymic, 16);
	printf("\n");

	printf("eapolframe is:");
	if (opt->nonstrict == 0) {
		lamont_hdump(cdata.eapolframe, 99);
	} else {
		lamont_hdump(cdata.eapolframe, 125);
	}

	printf("\n");

}

void printstats(struct timeval start, struct timeval end,
		unsigned long int wordcount)
{

	float elapsed = 0;

	if (end.tv_usec < start.tv_usec) {
		end.tv_sec -= 1;
		end.tv_usec += 1000000;
	}
	end.tv_sec -= start.tv_sec;
	end.tv_usec -= start.tv_usec;
	elapsed = end.tv_sec + end.tv_usec / 1000000.0;

	printf("\n%lu passphrases tested in %.2f seconds:  %.2f passphrases/"
	       "second\n", wordcount, elapsed, wordcount / elapsed);
	snprintf(mybuffer, sizeof(mybuffer), "\n%lu passphrases tested in %.2f seconds:  %.2f passphrases/"
	       "second\n", wordcount, elapsed, wordcount / elapsed);
        info_message(mybuffer, UPDATE_SUCCESS);
}

int nexthashrec(FILE * fp, struct hashdb_rec *rec)
{

	int recordlength, wordlen;

	if (fread(&rec->rec_size, sizeof(rec->rec_size), 1, fp) != 1) {
	
		perror("fread");
		return -1;
	}

	recordlength = abs(rec->rec_size);
	wordlen = recordlength - (sizeof(rec->pmk) + sizeof(rec->rec_size));

	if (wordlen > 63 || wordlen < 8) {
		fprintf(stderr, "Invalid word length: %d\n", wordlen);
		return -1;
	}

	/* hackity, hack, hack, hack */
	rec->word = password_buf;

	if (fread(rec->word, wordlen, 1, fp) != 1) {
		perror("fread");
		return -1;
	}

	if (fread(rec->pmk, sizeof(rec->pmk), 1, fp) != 1) {
		perror("fread");
		return -1;
	}

	return recordlength;
}

int nextdictword(char *word, FILE * fp)
{

	if (fgets(word, MAXPASSLEN + 1, fp) == NULL) {
		return (-1);
	}

	/* Remove newline */
	word[strlen(word) - 1] = '\0';

	if (feof(fp)) {
		return (-1);
	}

	return (strlen(word));
}

void hmac_hash(int ver, u8 *key, int hashlen, u8 *buf, int buflen, u8 *mic)
{
	u8 hash[SHA1_MAC_LEN];

	if (ver == WPA_KEY_INFO_TYPE_HMAC_MD5_RC4) {
		hmac_md5(key, hashlen, buf, buflen, mic);
	} else if (ver == WPA_KEY_INFO_TYPE_HMAC_SHA1_AES) {
		hmac_sha1(key, hashlen, buf, buflen, hash, NOCACHED);
		memcpy(mic, hash, MD5_DIGEST_LENGTH); /* only 16 bytes, not 20 */
	}
}

int hashfile_attack(struct user_opt *opt, char *passphrase, 
	struct crack_data *cdata)
{
	
	FILE *fp;
	int reclen, wordlen;
	u8 pmk[32];
	u8 ptk[64];
	u8 keymic[16];
	struct wpa_ptk *ptkset;
	struct hashdb_rec rec;
	struct hashdb_head hf_head;
	char headerssid[33];

	/* Open the hash file */
	if (*opt->hashfile == '-') {
		printf("Using STDIN for hashfile contents.\n");
		fp = stdin;
	} else {
		fp = fopen(opt->hashfile, "rb");
		if (fp == NULL) {
			perror("fopen");
			return(-1);
		}
	}

	/* Read the record header contents */
	if (fread(&hf_head, sizeof(hf_head), 1, fp) != 1) {
		perror("fread");
		return(-1);
	}

	/* Ensure selected SSID matches what's stored in the header record */
	if (memcmp(hf_head.ssid, opt->ssid, hf_head.ssidlen) != 0) {
		
		memcpy(&headerssid, hf_head.ssid, hf_head.ssidlen);
		headerssid[hf_head.ssidlen] = 0; /* NULL terminate string */

		fprintf(stderr, "\nSSID in hashfile (\"%s\") does not match "
			"SSID specified on the \n"
			"command line (\"%s\").  You cannot "
			"mix and match SSID's for this\nattack.\n\n",
			headerssid, opt->ssid);

		snprintf(mybuffer, sizeof(mybuffer), "SSID in hashfile (\"%s\") does not match "
			"SSID specified on the "
			"command line (\"%s\").  You cannot "
			"mix and match SSID's for this attack.\n\n",
			headerssid, opt->ssid);
        info_message(mybuffer, UPDATE_ERROR);
		return(-1);
	}


	while (feof(fp) == 0 && sig == 0) {

		/* Populate the hashdb_rec with the next record */
		reclen = nexthashrec(fp, &rec);

		/* nexthashrec returns the length of the record, test to ensure
		   passphrase is greater than 8 characters */
		wordlen = rec.rec_size - 
			(sizeof(rec.pmk) + sizeof(rec.rec_size));
		if (wordlen < 8) {
			printf("Found a record that was too short, this "
				"shouldn't happen in practice!\n");
			return(-1);
		}

		/* Populate passphrase with the record contents */
		memcpy(passphrase, rec.word, wordlen);

		/* NULL terminate passphrase string */
		passphrase[wordlen] = 0;

		if (opt->verbose > 1) {
			printf("Testing passphrase: %s\n", passphrase);
		}

		/* Increment the words tested counter */
		wordstested++;

		/* Status display */
		if ((wordstested % 10000) == 0) {
            if(stop_attack_cowp)
			return 1;
			//printf("key no. %ld: %s\n", wordstested, passphrase);
			//fflush(stdout);
			snprintf(mybuffer, sizeof(mybuffer), "Testing key no. %ld\n", wordstested);
        info_message(mybuffer, UPDATE_RUNNING);
		}

		if (opt->verbose > 1) {
			printf("Calculating PTK for \"%s\".\n", passphrase);
		}
		
		if (opt->verbose > 2) {
			printf("PMK is");
			lamont_hdump(pmk, sizeof(pmk));
		}

		if (opt->verbose > 1) {
			printf("Calculating PTK with collected data and "
			       "PMK.\n");
		}

		wpa_pmk_to_ptk(rec.pmk, cdata->aa, cdata->spa, cdata->anonce,
			       cdata->snonce, ptk, sizeof(ptk));

		if (opt->verbose > 2) {
			printf("Calculated PTK for \"%s\" is", passphrase);
			lamont_hdump(ptk, sizeof(ptk));
		}

		ptkset = (struct wpa_ptk *)ptk;

		if (opt->verbose > 1) {
			printf("Calculating hmac-MD5 Key MIC for this "
			       "frame.\n");
		}

		if (opt->nonstrict == 0) {
			hmac_hash(cdata->ver, ptkset->mic_key, 16, cdata->eapolframe,
				sizeof(cdata->eapolframe), keymic);
		} else {
			hmac_hash(cdata->ver, ptkset->mic_key, 16, cdata->eapolframe,
			 cdata->eapolframe_size, keymic);
		}

		if (opt->verbose > 2) {
			printf("Calculated MIC with \"%s\" is", passphrase);
			lamont_hdump(keymic, sizeof(keymic));
		}

		if (memcmp(&cdata->keymic, &keymic, sizeof(keymic)) == 0) {
			return 0;
		} else {
			continue;
		}
	}

	return 1;
}

int dictfile_attack(struct user_opt *opt, char *passphrase, 
	struct crack_data *cdata)
{
	
	FILE *fp;
	int fret;
	u8 pmk[32];
	u8 ptk[64];
	u8 keymic[16];
	struct wpa_ptk *ptkset;

	/* Open the dictionary file */
	if (*opt->dictfile == '-') {
		printf("Using STDIN for words.\n");
		fp = stdin;
	} else {
		fp = fopen(opt->dictfile, "r");
		if (fp == NULL) {
			perror("fopen");
			snprintf(mybuffer, sizeof(mybuffer), "Cant open dict file!\n");
        info_message(mybuffer, UPDATE_ERROR);

		}
	}


	while (feof(fp) == 0 && sig == 0) {

		/* Populate "passphrase" with the next word */
		fret = nextdictword(passphrase, fp);
		if (fret < 0) {
			break;
		}

		if (opt->verbose > 1) {
			printf("Testing passphrase: %s\n", passphrase);
		}

		/*
		 * Test length of word.  IEEE 802.11i indicates the passphrase
		 * must be at least 8 characters in length, and no more than 63 
		 * characters in length. 
		 */
		if (fret < 8 || fret > 63) {
			if (opt->verbose) {
				printf("Invalid passphrase length: %s (%u).\n",
				       passphrase, strlen(passphrase));
				snprintf(mybuffer, sizeof(mybuffer), "Invalid passphrase length: %s (%u).\n",
				       passphrase, strlen(passphrase));
        info_message(mybuffer, UPDATE_ERROR);
			}
			continue;
		} else {
			/* This word is good, increment the words tested
			counter */
			wordstested++;
		}
		if(stop_attack_cowp)
			return 1;
		/* Status display */
		if ((wordstested % 1000) == 0) {
			printf("key no. %ld: %s\n", wordstested, passphrase);
			fflush(stdout);
			snprintf(mybuffer, sizeof(mybuffer), "Testing key no. %ld\n", wordstested);
        info_message(mybuffer, UPDATE_RUNNING);
		}

		if (opt->verbose > 1) {
			printf("Calculating PMK for \"%s\".\n", passphrase);
		}
		
		pbkdf2_sha1(passphrase, opt->ssid, strlen(opt->ssid), 4096,
			    pmk, sizeof(pmk), USECACHED);

		if (opt->verbose > 2) {
			printf("PMK is");
			lamont_hdump(pmk, sizeof(pmk));
		}

		if (opt->verbose > 1) {
			printf("Calculating PTK with collected data and "
			       "PMK.\n");
		}

		wpa_pmk_to_ptk(pmk, cdata->aa, cdata->spa, cdata->anonce,
			       cdata->snonce, ptk, sizeof(ptk));

		if (opt->verbose > 2) {
			printf("Calculated PTK for \"%s\" is", passphrase);
			lamont_hdump(ptk, sizeof(ptk));
		}

		ptkset = (struct wpa_ptk *)ptk;

		if (opt->verbose > 1) {
			printf("Calculating hmac-MD5 Key MIC for this "
			       "frame.\n");
		}

		if (opt->nonstrict == 0) {
			hmac_hash(cdata->ver, ptkset->mic_key, 16, cdata->eapolframe,
				sizeof(cdata->eapolframe), keymic);
		} else {
			hmac_hash(cdata->ver, ptkset->mic_key, 16, cdata->eapolframe,
				cdata->eapolframe_size, keymic);
                }

		if (opt->verbose > 2) {
			printf("Calculated MIC with \"%s\" is", passphrase);
			snprintf(mybuffer, sizeof(mybuffer), "Calculated MIC with \"%s\" is", passphrase);
        info_message(mybuffer, UPDATE_RUNNING);
			lamont_hdump(keymic, sizeof(keymic));
		}

		if (memcmp(&cdata->keymic, &keymic, sizeof(keymic)) == 0) {
			return 0;
		} else {
			continue;
		}
	}

	return 1;
}

extern void stop_attack_cowpatty(int stop_att) {
	stop_attack_cowp = stop_att;
}

    extern int attack_cowpatty(JNIEnv* env, jobject thiz, jmethodID mid, char* dict_or_hash_file, int use_hash_file, char* pcap_file, char* essid, int myid)
    {
        struct user_opt opt;
        struct crack_data cdata;
        struct capture_data capdata;
        struct wpa_eapol_key *eapkeypacket;
        u8 eapolkey_nomic[99];
        struct timeval start, end;
        int ret;
        char passphrase[MAXPASSLEN + 1];
		stop_attack_cowp = 0;
		p = NULL;
		sig = 0;			
		wordstested = 0;
		
        setup_info_message(env, thiz, mid, myid);

        printf("%s %s - WPA-PSK dictionary attack. <jwright@hasborg.com>\n",
               PROGNAME, VER);

        memset(&opt, 0, sizeof(struct user_opt));
        memset(&capdata, 0, sizeof(struct capture_data));
        memset(&cdata, 0, sizeof(struct crack_data));
        memset(&eapolkey_nomic, 0, sizeof(eapolkey_nomic));

        if(use_hash_file)
			strncpy(opt.hashfile, dict_or_hash_file, sizeof(opt.hashfile));
		else
			strncpy(opt.dictfile, dict_or_hash_file, sizeof(opt.dictfile));

		strncpy(opt.pcapfile, pcap_file, sizeof(opt.pcapfile));

		strncpy(opt.ssid, essid, sizeof(opt.ssid));

		opt.nonstrict++;
        
        /* Collect and test command-line arguments */
        //parseopts(&opt, argc, argv);
        if(testopts(&opt) == -1)
        	return -1;
        printf("\n");

        /* Populate capdata struct */
        strncpy(capdata.pcapfilename, opt.pcapfile,
            sizeof(capdata.pcapfilename));
        if (openpcap(&capdata) != 0) {
            printf("Unsupported or unrecognized pcap file.\n");
            snprintf(mybuffer, sizeof(mybuffer), "Unsupported or unrecognized pcap file.\n");
        info_message(mybuffer, UPDATE_ERROR);
            return -1;
        }

        /* populates global *packet */
        while (getpacket(&capdata) > 0) {
            if (opt.verbose > 2) {
                lamont_hdump(packet, h->len);
            }
            /* test packet for data that we are looking for */
            if (memcmp(&packet[capdata.l2type_offset], DOT1X_LLCTYPE, 2) ==
                0 && (h->len >
                capdata.l2type_offset + sizeof(struct wpa_eapol_key))) {
                /* It's a dot1x frame, process it */
                handle_dot1x(&cdata, &capdata, &opt);
                if (cdata.aaset && cdata.spaset && cdata.snonceset &&
                    cdata.anonceset && cdata.keymicset
                    && cdata.eapolframeset) {
                    /* We've collected everything we need. */
                    break;
                }
            }
        }

        closepcap(&capdata);

        if (!(cdata.aaset && cdata.spaset && cdata.snonceset &&
              cdata.anonceset && cdata.keymicset && cdata.eapolframeset)) {
            printf("End of pcap capture file, incomplete four-way handshake "
                   "exchange.  Try using a\ndifferent capture.\n");

        snprintf(mybuffer, sizeof(mybuffer), "End of pcap capture file, incomplete four-way handshake "
                   "exchange. Try using a different capture.\n");
        info_message(mybuffer, UPDATE_ERROR);
            return -1;
        } else {
            if (cdata.ver == WPA_KEY_INFO_TYPE_HMAC_SHA1_AES) {
                printf("Collected all necessary data to mount crack"
                            " against WPA2/PSK passphrase.\n");

                snprintf(mybuffer, sizeof(mybuffer), "Collected all necessary data to mount crack"
                            " against WPA2/PSK passphrase.\n");
        info_message(mybuffer, UPDATE_SUCCESS);
             } else if (cdata.ver == WPA_KEY_INFO_TYPE_HMAC_MD5_RC4) {
                printf("Collected all necessary data to mount crack"
                            " against WPA/PSK passphrase.\n");

                snprintf(mybuffer, sizeof(mybuffer), "Collected all necessary data to mount crack"
                            " against WPA/PSK passphrase.\n");
        info_message(mybuffer, UPDATE_SUCCESS);
            }
        }

        if (opt.verbose > 1) {
            dump_all_fields(cdata, &opt);
        }

        if (opt.checkonly) {
            /* Don't attack the PSK, just return non-error return code */
            return 0;
        }

	/* Zero mic and length data for hmac-md5 calculation */
	eapkeypacket =
	    (struct wpa_eapol_key *)&cdata.eapolframe[EAPDOT1XOFFSET];
	memset(&eapkeypacket->key_mic, 0, sizeof(eapkeypacket->key_mic));
	if (opt.nonstrict == 0) {
		eapkeypacket->key_data_length = 0;
	}

	printf("Starting dictionary attack.  Please be patient.\n");
	fflush(stdout);

	snprintf(mybuffer, sizeof(mybuffer), "Starting dictionary attack.  Please be patient.\n");
        info_message(mybuffer, UPDATE_RUNNING);

	signal(SIGINT, cleanup);
	signal(SIGTERM, cleanup);
	signal(SIGQUIT, cleanup);

	gettimeofday(&start, 0);

	if (!IsBlank(opt.hashfile)) {
		ret = hashfile_attack(&opt, passphrase, &cdata);
	} else if (!IsBlank(opt.dictfile)) {
		ret = dictfile_attack(&opt, passphrase, &cdata);
	} else {
		usage("Must specify dictfile or hashfile (-f or -d)");
		

		snprintf(mybuffer, sizeof(mybuffer), "Must specify dictfile or hashfile\n");
        info_message(mybuffer, UPDATE_ERROR);
        return 1;
	}

	if (ret == 0) {
		printf("\nThe PSK is \"%s\".\n", passphrase);
		snprintf(mybuffer, sizeof(mybuffer), "\nThe PSK is \"%s\".\n", passphrase);
        info_message(mybuffer, UPDATE_SUCCESS);

		gettimeofday(&end, 0);
		printstats(start, end, wordstested);
		return 0;
	} else {
		printf("Unable to identify the PSK from the dictionary file. " 
	       		"Try expanding your\npassphrase list, and double-check"
		        " the SSID.  Sorry it didn't work out.\n");

		snprintf(mybuffer, sizeof(mybuffer), "Unable to identify the PSK from the dictionary file. " 
	       		"Try expanding your passphrase list, and double-check"
		        " the SSID.  Sorry it didn't work out.\n");
        info_message(mybuffer, UPDATE_ERROR);
		gettimeofday(&end, 0);
		printstats(start, end, wordstested);
		return 1;
	}

	return 1;

}
