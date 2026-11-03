#define _GNU_SOURCE
#define _XOPEN_SOURCE 700


#include <argp.h>
#include <string.h>
#include <byteswap.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <linux/netlink.h>
#include <pcap.h>


#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sys/prctl.h>
#include <string.h>

#define AS_STR2(TEXT) #TEXT
#define AS_STR(TEXT) AS_STR2(TEXT)

#define NETLINK_USER 31
#define MAX_PAYLOAD 1024 /* maximum payload size*/

char *pcap_file_name = NULL;
int listen_port = 5556;

int pid = -1;
pthread_t p1;
const char *argp_program_version = VERSION;
const char *argp_program_bug_address = "<fknapp@seemoo.tu-darmstadt.de>";

static char doc[] = "rawproxyreverse -- program to open a raw socket and inject frames as UDP datagrams.";

static struct argp_option options[] = {

	{"interface", 'i', "INTERFACE", 0, "Interface used as RAW interface."},
	{"parent", 'p', "PARENT", 0, "Parent process id to monitor."},
	{"port", 'P', "PORT", 0, "UDP port to listen on (default: 5556)."},
	{ 0 }
};

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
	switch (key) {

		case 'i':
			pcap_file_name = arg;
			break;

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

		case 'P': {
			char *endptr = NULL;
			long v;
			errno = 0;
			v = strtol(arg, &endptr, 10);
			if (endptr == arg || *endptr != '\0' || errno != 0 || v <= 0 || v > 65535) {
				fprintf(stderr, "ERR: invalid port '%s'\n", arg);
				exit(EXIT_FAILURE);
			}
			listen_port = (int) v;
			break;
		}
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

void *check_parent(void *unused)
{
	(void) unused;
	if(pid != -1) {
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
	pcap_t *handle;
	unsigned char packet[65536];
	char errbuf[PCAP_ERRBUF_SIZE];
	struct sockaddr_in si_app;
	socklen_t si_app_len;
	int sock, ret, recv_len;
	int opt = 1;

	argp_parse(&argp, argc, argv, 0, 0, 0);

	if(pcap_file_name == NULL) {
		fprintf(stderr, "ERR: no interface given (use -i)\n");
		exit(EXIT_FAILURE);
	}

	prctl(PR_SET_PDEATHSIG, SIGTERM);

	pthread_create(&p1, NULL, check_parent, (void *)NULL);
	pthread_detach(p1);


	handle = pcap_open_live(pcap_file_name, 65535, 1, 1000, errbuf);
	if(handle == NULL) {
		fprintf(stderr, "ERR: pcap_open_live(%s): %s\n", pcap_file_name, errbuf);
		exit(EXIT_FAILURE);
	}

	/* Trust model: this endpoint injects whatever UDP payload arrives onto the
	 * radio without authentication. It binds to 127.0.0.1 only, so the attack
	 * surface is limited to local processes/users on this host, not the
	 * network. Do not bind this to a routable address. */
	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port = htons(listen_port),
		.sin_addr.s_addr = inet_addr("127.0.0.1"),
		.sin_zero = { 0 }
	};

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock < 0) {
		perror("ERR: socket");
		exit(EXIT_FAILURE);
	}
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		perror("ERR: bind");
		exit(EXIT_FAILURE);
	}

	while(1) {
		si_app_len = sizeof(si_app);
		recv_len = recvfrom(sock, packet, sizeof(packet), MSG_TRUNC, (struct sockaddr *) &si_app, &si_app_len);
		/* a negative length must never reach pcap_inject(): its length
		 * argument is size_t, so -1 would become SIZE_MAX and read far past
		 * the packet buffer */
		if(recv_len < 0) {
			perror("ERR: recvfrom");
			continue;
		}
		if((size_t) recv_len > sizeof(packet)) {
			fprintf(stderr, "WARN: packet truncated (%d > %zu)\n", recv_len, sizeof(packet));
			continue;
		}
		printf("Successfully received %d bytes\n", recv_len);

		ret = pcap_inject(handle, packet, recv_len);
		if(ret != -1)
			printf("Successfully injected %d bytes\n", ret);
		else
			printf("pcap_inject error: %s\n", pcap_geterr(handle));

	}

	exit(EXIT_SUCCESS);
}
