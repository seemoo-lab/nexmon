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

#include <unistd.h>

#define AS_STR2(TEXT) #TEXT
#define AS_STR(TEXT) AS_STR2(TEXT)

#define NETLINK_USER 31
#define MAX_PAYLOAD 1024 /* maximum payload size*/

char *pcap_file_name = NULL;


int pid = -1;
pthread_t p1;
const char *argp_program_version = VERSION;
const char *argp_program_bug_address = "<fknapp@seemoo.tu-darmstadt.de>";

static char doc[] = "rawproxyreverse -- program to open a raw socket and inject frames as UDP datagrams.";

static struct argp_option options[] = {

	{"interface", 'i', "INTERFACE", 0, "Interface used as RAW interface."},
	{"parent", 'p', "PARENT", 0, "Parent process id to monitor."},
	{ 0 }
};

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
	switch (key) {

		case 'i':
			pcap_file_name = arg;
			break;

		case 'p':
			pid = atoi(arg);
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
	pcap_t *handle;
	unsigned char packet[1500];
	char errbuf[PCAP_ERRBUF_SIZE];
	struct sockaddr_in si_app;
	socklen_t si_app_len = sizeof(si_app);
	int sock, ret, recv_len;

	argp_parse(&argp, argc, argv, 0, 0, 0);

    pthread_create(&p1, NULL, check_parent, (void *)NULL);


	handle = pcap_open_live(pcap_file_name, BUFSIZ, 1, 1000, errbuf);
	struct sockaddr_in sin = {
		.sin_family = AF_INET,
		.sin_port = htons(5556),
		.sin_addr.s_addr = inet_addr("127.0.0.1"),
		.sin_zero = { 0 }
	};

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	bind(sock, (struct sockaddr *)&sin, sizeof(sin));
	
	while(1) {
		recv_len = recvfrom(sock, packet, 1500, 0, (struct sockaddr *) &si_app, &si_app_len);
		printf("Successful recevied %d bytes\n", recv_len);


		
		ret = pcap_inject(handle, &packet, recv_len);
		if(ret != -1)
			printf("Successful injected %d bytes\n", ret);
		else
			printf("pcap_inject error");

	}

	
	exit(EXIT_SUCCESS);
}
