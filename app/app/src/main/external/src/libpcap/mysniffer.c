#include <stdio.h>
#include <pcap.h>
#include <jni.h>
#include "mysniffer.h"
#include <stdlib.h>

JNIEnv *cb_jnienv;
jobject cb_obj;
jmethodID cb_mid;
u_char packetArray;
pcap_t *handle;
int running;

typedef struct {
    JNIEnv *env;
    jobject pThis;
    jmethodID mid;
} Configuration;


void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {

	jbyteArray _header, payload;

    int len_header = 0;
        if(running) {
    len_header = header->caplen;
	payload = (*cb_jnienv)->NewByteArray(cb_jnienv, len_header);
	_header = (*cb_jnienv)->NewByteArray(cb_jnienv, sizeof(struct pcap_pkthdr));

	(*cb_jnienv)->SetByteArrayRegion(cb_jnienv, payload, 0, (header->caplen), (jbyte*)packet);
	(*cb_jnienv)->SetByteArrayRegion(cb_jnienv, _header, 0, sizeof(struct pcap_pkthdr), (jbyte*)header);


	    (*cb_jnienv)->CallVoidMethod(cb_jnienv, cb_obj, cb_mid, payload, _header);

	(*cb_jnienv)->DeleteLocalRef(cb_jnienv, payload);
	(*cb_jnienv)->DeleteLocalRef(cb_jnienv, _header);
    }

}

/*
 * print data in rows of 16 bytes: offset   hex   ascii
 *
 * 00000   47 45 54 20 2f 20 48 54  54 50 2f 31 2e 31 0d 0a   GET / HTTP/1.1..
 */
void
print_hex_ascii_line(const u_char *payload, int len, int offset)
{

	int i;
	int gap;
	const u_char *ch;

	/* offset */
	printf("%05d   ", offset);
	
	/* hex */
	ch = payload;
	for(i = 0; i < len; i++) {
		printf("%02x ", *ch);
		ch++;
		/* print extra space after 8th byte for visual aid */
		if (i == 7)
			printf(" ");
	}
	/* print space to handle line less than 8 bytes */
	if (len < 8)
		printf(" ");
	
	/* fill hex gap with spaces if not full line */
	if (len < 16) {
		gap = 16 - len;
		for (i = 0; i < gap; i++) {
			printf("   ");
		}
	}
	printf("   ");
	
	/* ascii (if printable) */
	ch = payload;
	for(i = 0; i < len; i++) {
		if (isprint(*ch))
			printf("%c", *ch);
		else
			printf(".");
		ch++;
	}

	printf("\n");

return;
}

char * sniff_it(JNIEnv *env, jobject pThis, jmethodID mid) {

	char *dev = "wlan0";
	char errbuf[PCAP_ERRBUF_SIZE];
	//struct pcap_pkthdr header;
	const u_char *packet;
	int prot = -1;
	cb_jnienv = env;
	cb_obj = (*env)->NewGlobalRef(env, pThis);
	cb_mid = mid;

    Configuration conf[1] = {
      {env, pThis, mid}
    };

	//cb_obj = pThis;
	//cb_mid = mid;


	//printf("Device: %s\n", dev);

	handle = pcap_open_live(dev, BUFSIZ, 1, 100, errbuf);

	if(handle == NULL) {
		fprintf(stderr, "Couldnt open device %s: %s\n", dev, errbuf);
		return("error");
	}
	 pcap_setnonblock(handle, 1, errbuf);


	prot = pcap_datalink(handle);
    running = 1;

    while(running)
	    pcap_dispatch(handle, -1, got_packet, NULL);

pcap_close(handle);
     (*env)->DeleteGlobalRef(env, cb_obj);
	return("sniff it executed");
}



void stop_sniffer(void) {
	    running = 0;
}