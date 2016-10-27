#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include <string.h>
#include <byteswap.h>
#include "darm/darm.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define ROM_SIZE (640*1024)
#define RAM_SIZE (768*1024)
#define RAM_START (0x180000)

#define AS_STR2(TEXT) #TEXT
#define AS_STR(TEXT) AS_STR2(TEXT)

char *ram_array = NULL;
long ram_len = 0;

char *ram_file_name = AS_STR(RAM_FILE_NAME);
int fp_config_base = 0;
int fp_config_end = 0;

const char *argp_program_version = "fpext";
const char *argp_program_bug_address = "<mschulz@seemoo.tu-darmstadt.de>";

static char doc[] = "fpext -- a program to extract flash patches form a firmware rom.";

static struct argp_option options[] = {
	{"ramfile", 'r', "FILE", 0, "Read ram from FILE"},
	{"fpconfigbase", 'b', "ADDR", 0, "Use ADDR as base address of the flash patch config block"},
	{"fpconfigend", 'e', "ADDR", 0, "Use ADDR as end address of the flash patch config block"},
	{ 0 }
};

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
	switch (key) {

		case 'r':
			ram_file_name = arg;
			break;

		case 'b':
			fp_config_base = strtol(arg, NULL, 0);
			break;

		case 'e':
			fp_config_end = strtol(arg, NULL, 0);
			break;
		
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };

unsigned char ethernet_ipv6_udp_header_array[] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,   /* ETHERNET: Destination MAC Address */
  'N', 'E', 'X', 'M', 'O', 'N',         /* ETHERNET: Source MAC Address */
  0x86, 0xDD,                           /* ETHERNET: Type */
  0x60, 0x00, 0x00, 0x00,               /* IPv6: Version / Traffic Class / Flow Label */
  0x00, 0x08,                           /* IPv6: Payload Length */
  0x88,                                 /* IPv6: Next Header = UDPLite */
  0x01,                                 /* IPv6: Hop Limit */
  0xFF, 0x02, 0x00, 0x00,               /* IPv6: Source IP */
  0x00, 0x00, 0x00, 0x00,               /* IPv6: Source IP */
  0x00, 0x00, 0x00, 0x00,               /* IPv6: Source IP */
  0x00, 0x00, 0x00, 0x01,               /* IPv6: Source IP */
  0xFF, 0x02, 0x00, 0x00,               /* IPv6: Destination IP */
  0x00, 0x00, 0x00, 0x00,               /* IPv6: Destination IP */
  0x00, 0x00, 0x00, 0x00,               /* IPv6: Destination IP */
  0x00, 0x00, 0x00, 0x01,               /* IPv6: Destination IP */
  0xD6, 0xD8,                           /* UDPLITE: Source Port = 55000 */
  0xD6, 0xD8,                           /* UDPLITE: Destination Port = 55000 */
  0x00, 0x08,                           /* UDPLITE: Checksum Coverage */
  0x52, 0x46,                           /* UDPLITE: Checksum only over UDPLITE header*/
};

int
read_file_to_array(char *filename, char **buffer, long *filelen)
{
	FILE *fileptr;

	if((fileptr = fopen(filename, "rb"))) {
		fseek(fileptr, 0, SEEK_END);
		*filelen = ftell(fileptr);
		rewind(fileptr);

		*buffer = (char *) malloc(*filelen + 1);
		fread(*buffer, *filelen, 1, fileptr);
		fclose(fileptr);

		return *filelen;
	}

	return 0;
}

int
get_words(unsigned int addr, unsigned short *low, unsigned short *high)
{
	if (addr > RAM_START && addr < RAM_START + ram_len - 4) {
		*low = *((unsigned short *) (ram_array + addr - RAM_START));
		*high = *((unsigned short *) (ram_array + addr - RAM_START + 2));
		return 2;
	}

	return 0;
}

struct fp_config {
	unsigned int target_addr;
	unsigned int size;
	unsigned int data_ptr;
};

void
analyse_ram()
{
	darm_t d;
	darm_t *dd = &d;
	unsigned short low, high;
	
	struct fp_config *fpc = (struct fp_config *) (ram_array + fp_config_base - RAM_START);

	darm_init(&d);

	for (int i = 0; i < (fp_config_end - fp_config_base) / sizeof(struct fp_config); i++) {
		get_words(fpc[i].data_ptr, &low, &high);
		darm_disasm(dd, low, high, 1);

		printf("__attribute__((at(0x%08x, \"flashpatch\")))\n", fpc[i].target_addr);
		printf("BPatch(flash_patch_%d, 0x%08x);\n\n", i, fpc[i].target_addr + dd->imm + 4);
	}
}

int
main(int argc, char **argv)
{
	argp_parse(&argp, argc, argv, 0, 0, 0);

	if(!read_file_to_array(ram_file_name, &ram_array, &ram_len)) {
		fprintf(stderr, "ERR: ram file empty or unavailable.\n");
		exit(EXIT_FAILURE);
	}

	analyse_ram();

	exit(EXIT_SUCCESS);
}
