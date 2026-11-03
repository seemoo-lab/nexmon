#define _GNU_SOURCE
#define _XOPEN_SOURCE 700
#ifndef USE_LIBPCAP
#define USE_LIBPCAP
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
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
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <sys/prctl.h>
#include <string.h>

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
		case 'p': {
			char *endptr = NULL;
			long v;
			errno = 0;
			v = strtol(arg, &endptr, 10);
			if (endptr == arg || *endptr != '\0' || errno != 0 || v <= 0 || v > INT_MAX) {
				fprintf(stderr, "ERR: invalid parent PID '%s'\n", arg);
				exit(EXIT_FAILURE);
			}
			pid = (int) v;
			break;
		}
		case 'o': {
			char *endptr = NULL;
			long v;
			errno = 0;
			v = strtol(arg, &endptr, 10);
			if (endptr == arg || *endptr != '\0' || errno != 0 || v <= 0 || v > 65535) {
				fprintf(stderr, "ERR: invalid port '%s' (expected 1..65535)\n", arg);
				exit(EXIT_FAILURE);
			}
			portin = (int) v;
			break;
		}
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
/* Fixed-width 32-bit PCAP packet record header (16 bytes) for UDP wire
 * compatibility across 32-bit and 64-bit (LP64) builds. Native
 * struct pcap_pkthdr is 24 bytes on LP64 due to 64-bit struct timeval. */
struct pcap_pkthdr32 {
	uint32_t ts_sec;
	uint32_t ts_usec;
	uint32_t caplen;
	uint32_t len;
} __attribute__((packed));

typedef char pcap_pkthdr32_size_check[(sizeof(struct pcap_pkthdr32) == 16) ? 1 : -1];

void
analyse_pcap_file(char *filename)
{
	pcap_t *pcap;
	char errbuf[PCAP_ERRBUF_SIZE];
	const unsigned char *packet;
	struct pcap_pkthdr header;
	int sock;
	struct iovec iov[2];

	struct msghdr msg_hdr = { 0 };

	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port = htons(portin),
		.sin_addr.s_addr = inet_addr("127.0.0.1"),
		.sin_zero = { 0 }
	};


	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock < 0) {
		perror("ERR: socket");
		return;
	}
	/* Allow rapid reuse of the UDP port */
	int one = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	if(!strcmp(filename, "any")) {
		pcap = pcap_open_live("any", 65535, 1, 0, errbuf);
	} else {
		char live_errbuf[PCAP_ERRBUF_SIZE] = {0};
		/* Attempt live capture first (e.g. wlan0, mon0, wlan0mon) */
		pcap = pcap_open_live(filename, 65535, 1, 0, live_errbuf);
		if(!pcap) {
			/* Fall back to opening as an offline capture file */
			pcap = pcap_open_offline(filename, errbuf);
			if (!pcap) {
				fprintf(stderr, "ERR: pcap_open_live: %s; pcap_open_offline: %s\n", live_errbuf, errbuf);
			}
		}
	}

	if(!pcap) {
		if (errbuf[0]) fprintf(stderr, "ERR: %s\n", errbuf);
		else perror("ERR: pcap");
		close(sock);
		return;
	}


	unsigned int p_len;
	while ((packet = pcap_next(pcap, &header)) != NULL) {
		struct pcap_pkthdr32 wire_hdr;
		unsigned int max_payload;
		unsigned int stripped = 0;

		p_len = header.caplen;

		if(discard_radiotap) {
			int rtap_len;
			/* need at least the 8-byte radiotap header (version, pad, len, present) */
			if (header.caplen < 8)
				continue;
			rtap_len = ((packet[3] & 0xff) << 8) | (packet[2] & 0xff);
			/* a firmware/attacker-controlled length must be >= 8 and stay within
			 * the captured bytes, otherwise packet is malformed or would point
			 * past the buffer */
			if (rtap_len < 8 || (unsigned int) rtap_len > header.caplen)
				continue;
			packet = packet + rtap_len;
			stripped = (unsigned int) rtap_len;
			p_len = header.caplen - stripped;
		}

		max_payload = discard_pcap
			? 65507u
			: (65507u - (unsigned int) sizeof(struct pcap_pkthdr32));
		if (p_len > max_payload)
			p_len = max_payload;

		if(discard_pcap) {
			iov[0].iov_base = (caddr_t) packet;
			iov[0].iov_len = p_len;
			msg_hdr.msg_iovlen = 1;
		} else {
			wire_hdr.ts_sec = (uint32_t) header.ts.tv_sec;
			wire_hdr.ts_usec = (uint32_t) header.ts.tv_usec;
			wire_hdr.caplen = (uint32_t) p_len;
			wire_hdr.len = (uint32_t) ((header.len >= stripped) ? (header.len - stripped) : p_len);
			iov[0].iov_base = (caddr_t) &wire_hdr;
			iov[0].iov_len = sizeof(struct pcap_pkthdr32);
			iov[1].iov_base = (caddr_t) packet;
			iov[1].iov_len = p_len;
			msg_hdr.msg_iovlen = 2;
		}

		msg_hdr.msg_name = (caddr_t) &sin;
		msg_hdr.msg_namelen = sizeof(sin);
		msg_hdr.msg_iov = iov;

		if(sendmsg(sock, &msg_hdr, 0) < 0)
			fprintf(stderr, "ERR: sendmsg: %s\n", strerror(errno));
	}
	{
		const char *perr = pcap_geterr(pcap);
		if (perr && perr[0]) fprintf(stderr, "WARN: pcap: %s\n", perr);
	}

	pcap_close(pcap);
	close(sock);
}
#endif

void *check_parent(void *unused)
{
	(void) unused;
	if(pid != -1) {
		/* If parent dies, kernel sends SIGTERM via PR_SET_PDEATHSIG — handles
		 * PID reuse race where kill(pid,0) would otherwise watch a recycled PID */
		prctl(PR_SET_PDEATHSIG, SIGTERM);
		/* Check immediately after setting death signal in case parent already died */
		if (kill(pid, 0) != 0) exit(EXIT_SUCCESS);
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

	prctl(PR_SET_PDEATHSIG, SIGTERM);

	pthread_create(&p1, NULL, check_parent, (void *)NULL);
	pthread_detach(p1);

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
