#define _XOPEN_SOURCE 700
#define USE_LIBPCAP

#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include <string.h>
#include <byteswap.h>
#include <pthread.h>
#ifdef USE_LIBPCAP
#include <pcap.h>
#endif

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define AS_STR2(TEXT) #TEXT
#define AS_STR(TEXT) AS_STR2(TEXT)

#ifdef USE_LIBPCAP
char *pcap_file_name = NULL;
#endif

int pid = -1;
int portin = 5555;
int discard_pcap = 0;
int discard_radiotap = 0;
pthread_t p1;
const char *argp_program_version = VERSION;
const char *argp_program_bug_address = "<mschulz@seemoo.tu-darmstadt.de>";

static char doc[] = "rawproxy -- program to open a raw socket and forward frames as UDP datagrams.";

static struct argp_option options[] = {
#ifdef USE_LIBPCAP
	{"interface", 'i', "INTERFACE", 0, "Interface used as RAW interface."},
#endif
	{"parent", 'p', "PID", 0, "Parent process id to monitor (default no monitor)."},
	{"out", 'o', "PORT", 0, "Port to use (default: 5555)."},
	{"header", 'h', 0, 0, "Discard pcap header."},
	{"radiotap", 'r', 0, 0, "Discard radiotap header."},
	{ 0 }
};

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
	switch (key) {
#ifdef USE_LIBPCAP
		case 'i':
			pcap_file_name = arg;
			break;
#endif
		case 'p':
			pid = atoi(arg);
			break;
		case 'o':
			portin = atoi(arg);
			break;
		case 'h':
			discard_pcap = 1;
			break;
		case 'r':
			discard_radiotap = 1;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

struct map {
	struct map *next;
	unsigned int addr;
	char *name;
};

static struct argp argp = { options, parse_opt, 0, doc };

#ifdef USE_LIBPCAP
void
analyse_pcap_file(char *filename)
{
	pcap_t *pcap;
	char errbuf[PCAP_ERRBUF_SIZE];
	const unsigned char *packet;
	struct pcap_pkthdr header;
	struct bpf_program fp;
	char *fct_name;
	int offset = 0, sock;
	struct iovec iov[2];

	struct msghdr msg_hdr;

	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port = htons(portin),
		.sin_addr.s_addr = inet_addr("127.0.0.1"),
		.sin_zero = { 0 }
	};


	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(!sock) {
		printf("ERR: socket\n");
		return;
	}

	if(!strcmp(filename, "wlan0")) {
		pcap = pcap_open_live("wlan0", BUFSIZ, 1, 0, errbuf);
	} else if(!strcmp(filename, "any")) {
		pcap = pcap_open_live("any", BUFSIZ, 1, 0, errbuf);
		offset = 2;
	} else {
		pcap = pcap_open_offline(filename, errbuf);	
	}

	if(!pcap) {
		printf("ERR: %s\n", errbuf);
		return;
	}


	int i = 0;
	int p_len;
	while ((packet = pcap_next(pcap, &header)) != NULL) {
		
		if(discard_radiotap) {
			int rtap_len = ((packet[3] & 0xff) << 8) | (packet[2] & 0xff);
			packet = packet + rtap_len;
		}


		if(discard_pcap) {
			iov[0].iov_base = (caddr_t) packet;
			iov[0].iov_len = header.caplen;
			msg_hdr.msg_iovlen = 1;
		} else {
			iov[0].iov_base = (caddr_t) &header;
			iov[0].iov_len = sizeof(struct pcap_pkthdr);
			iov[1].iov_base = (caddr_t) packet;
			iov[1].iov_len = header.caplen;
			msg_hdr.msg_iovlen = 2;
		}
		
		msg_hdr.msg_name = (caddr_t) &sin;
		msg_hdr.msg_namelen = sizeof(sin);
		msg_hdr.msg_iov = iov;

		if(!sendmsg(sock, &msg_hdr, 0))
			printf("ERR: %d\n", errno);
	}
}
#endif

void *check_parent(void *unused)
{

	if(pid != -1) {
		while(1) {
			if(kill(pid, 0) != 0) {
				exit(EXIT_SUCCESS);
			} else {
				sleep(1);
			}
		}
	}
	return NULL;
}

int
main(int argc, char **argv)
{
	
	argp_parse(&argp, argc, argv, 0, 0, 0);

	pthread_create(&p1, NULL, check_parent, (void *)NULL);

#ifdef USE_LIBPCAP
	if (pcap_file_name)
		analyse_pcap_file(pcap_file_name);
#else
	if (0);
#endif
	else
		fprintf(stderr, "ERR: no source selected.\n");

	exit(EXIT_SUCCESS);
}
