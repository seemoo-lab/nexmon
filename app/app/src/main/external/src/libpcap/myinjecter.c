#include <stdio.h>
#include <pcap.h>
#include <jni.h>
#include "myinjecter.h"

#define UPDATE_ERROR 1
#define UPDATE_SUCCESS 2
#define UPDATE_RUNNING 3

int inject_it(const u_char *packet, int len, char *device, struct ret_msg *my_message) {
	start_logger("INJECTER LOGGER");
	pcap_t *handle;
	//struct ret_msg my_message;
	char *dev = device;
	char errbuf[PCAP_ERRBUF_SIZE];
	//struct pcap_pkthdr header;
	char frame[] = 
    {
        0x00, 0x00, 0x24, 0x00, 0x2f, 0x40, 0x00, 0xa0, /* Radiotap */
        0x20, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x96, 0x5c, 0xd7, 0x2e, 0x00, 0x00, 0x00, 0x00, 
        0x10, 0x02, 0x6c, 0x09, 0xa0, 0x00, 0xca, 0x00,
        0x00, 0x00, 0xca, 0x00,
        0x80, 0x00, 0x00, 0x00,                         /* 802.11: type management */
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 			/* destination address */
        0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc,				/* source address */
        0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 			/* BSS address */
        0x10, 0x00, 									/* sequence and fragment numbers */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	/* timestamp */
        0x64, 0x00, 0x21, 0x05, 
        0x00, 											/* tag number: SSID parameter set */
        0x06, 											/* tag length */
        'N', 'E', 'X', 'M', 'O', 'N',		 			/* SSID */
    };
	int prot = -1;

	handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
	if(handle == NULL) {
		fprintf(stderr, "Couldnt open device %s: %s\n", dev, errbuf);
		snprintf(my_message->msg, sizeof(my_message->msg), "Couldnt open device %s: %s\n", dev, errbuf);
        my_message->msg_type = UPDATE_ERROR;
		return -1;
	}

	int ret = pcap_inject(handle, packet, len);
    if (ret == -1) {
        fprintf(stderr, "Error on injecting packet!\n");
        snprintf(my_message->msg, sizeof(my_message->msg),  "Error on injecting packet!\n");
        my_message->msg_type = UPDATE_ERROR;
        pcap_perror(handle, 0);
        pcap_close(handle);
        return ret;
    } else {
        printf("Successful injected %d bytes\n", ret);
    }

    pcap_close(handle);
    snprintf(my_message->msg, sizeof(my_message->msg),  "Successful injected %d bytes\n", ret);
    my_message->msg_type = UPDATE_SUCCESS;
	return 1;
}



