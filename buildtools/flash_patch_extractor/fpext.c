#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include <string.h>
#include "darm/darm.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <execinfo.h>
#include <signal.h>

#define AS_STR2(TEXT) #TEXT
#define AS_STR(TEXT) AS_STR2(TEXT)

char *ram_array = NULL;
long ram_len = 0;

char *rom_array = NULL;
long rom_len = 0;

char *ram_file_name = AS_STR(RAM_FILE_NAME);
char *rom_file_in_name = NULL;
char *rom_file_out_name = NULL;
int fp_config_base = 0;
int fp_config_end = 0;
int ram_start = 0x180000;
int rom_start = 0x0;
char bcm43596 = 0;
char bcm4366 = 0;

const char *argp_program_version = "fpext";
const char *argp_program_bug_address = "<mschulz@seemoo.tu-darmstadt.de>";

static char doc[] = "fpext -- a program to extract flash patches form a firmware rom.";

static struct argp_option options[] = {
	{"ramfile", 'r', "FILE", 0, "Read ram from FILE"},
	{"ramstart", 's', "ADDR", 0, "RAM start address"},
	{"fpconfigbase", 'b', "ADDR", 0, "Use ADDR as base address of the flash patch config block"},
	{"fpconfigend", 'e', "ADDR", 0, "Use ADDR as end address of the flash patch config block"},
	{"romfilein", 'i', "FILE", 0, "Apply patches to this ROM FILE"},
	{"romfileout", 'o', "FILE", 0, "Save the patched ROM file as FILE"},
	{"romstart", 't', "ADDR", 0, "ROM start address"},
	{"bcm43596", 'x', 0, 0, "Select whether target chip has a flash patching unit similar to the bcm43596"},
	{"bcm4366", 'y', 0, 0, "Select whether target chip has a flash patching unit similar to the bcm4366"},
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

		case 's':
			ram_start = strtol(arg, NULL, 0);
			break;

		case 'i':
			rom_file_in_name = arg;
			break;

		case 'o':
			rom_file_out_name = arg;
			break;

		case 't':
			rom_start = strtol(arg, NULL, 0);
			break;

		case 'x':
			bcm43596 = 1;
			break;

		case 'y':
			bcm4366 = 1;
			break;
		
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };

int
write_array_to_file(char *filename, char *buffer, long filelen)
{
	FILE *fileptr;

	if ((fileptr = fopen(filename, "wb"))) {
		fwrite(buffer, 1, filelen, fileptr);
		fclose(fileptr);
		return 0;
	}

	return -1;
}

int
read_file_to_array(char *filename, char **buffer, long *filelen)
{
	FILE *fileptr;

	if ((fileptr = fopen(filename, "rb"))) {
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
	if (addr > ram_start && addr < ram_start + ram_len - 4) {
		*low = *((unsigned short *) (ram_array + addr - ram_start));
		*high = *((unsigned short *) (ram_array + addr - ram_start + 2));
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
	
	struct fp_config *fpc = (struct fp_config *) (ram_array + fp_config_base - ram_start);

	darm_init(&d);

	for (int i = 0; i < (fp_config_end - fp_config_base) / sizeof(struct fp_config); i++) {
		get_words(fpc[i].data_ptr, &low, &high);
		darm_disasm(dd, low, high, 1);

		printf("__attribute__((weak))\n");
		printf("__attribute__((at(0x%08x, \"flashpatch\")))\n", fpc[i].target_addr);
		printf("BPatch(flash_patch_%d, 0x%08x);\n\n", i, fpc[i].target_addr + dd->imm + 4);

		if (rom_array != NULL && (fpc[i].target_addr - rom_start) < rom_len) {
			memcpy(&rom_array[fpc[i].target_addr - rom_start], &ram_array[fpc[i].data_ptr - ram_start], fpc[i].size);
		}
	}
}

struct fp_config_bcm43596 {
	unsigned int target_addr;
	unsigned int data_ptr;
};

void
analyse_ram_bcm43596()
{
	darm_t d;
	darm_t *dd = &d;
	unsigned short low, high;
	
	struct fp_config_bcm43596 *fpc = (struct fp_config_bcm43596 *) (ram_array + fp_config_base - ram_start);

	darm_init(&d);

	for (int i = 0; i < (fp_config_end - fp_config_base) / sizeof(struct fp_config_bcm43596); i++) {
		get_words(fpc[i].data_ptr, &low, &high);
		darm_disasm(dd, low, high, 1);

		printf("__attribute__((weak))\n");
		printf("__attribute__((at(0x%08x, \"flashpatch\")))\n", fpc[i].target_addr);
		printf("unsigned int flash_patch_%d[2] = {0x%08x, 0x%08x};\n\n", i, 
			*((unsigned int *) (ram_array + fpc[i].data_ptr - ram_start)), 
			*((unsigned int *) (ram_array + fpc[i].data_ptr + 4 - ram_start)));

		if (rom_array != NULL && (fpc[i].target_addr - rom_start) < rom_len) {
			memcpy(&rom_array[fpc[i].target_addr - rom_start], &ram_array[fpc[i].data_ptr - ram_start], 8);
		}
	}
}

void
analyse_ram_bcm4366()
{
	darm_t d;
	darm_t *dd = &d;
	unsigned short low, high;
	
	struct fp_config_bcm43596 *fpc = (struct fp_config_bcm43596 *) (ram_array + fp_config_base - ram_start);

	darm_init(&d);

	for (int i = 0; i < (fp_config_end - fp_config_base) / sizeof(struct fp_config_bcm43596); i++) {
		get_words(fpc[i].data_ptr, &low, &high);
		darm_disasm(dd, low, high, 1);

		printf("__attribute__((weak))\n");
		printf("__attribute__((at(0x%08x, \"flashpatch\")))\n", fpc[i].target_addr);
		printf("unsigned int flash_patch_%d[4] = {0x%08x, 0x%08x, 0x%08x, 0x%08x};\n\n", i, 
			*((unsigned int *) (ram_array + fpc[i].data_ptr - ram_start)), 
			*((unsigned int *) (ram_array + fpc[i].data_ptr + 4 - ram_start)), 
			*((unsigned int *) (ram_array + fpc[i].data_ptr + 8 - ram_start)), 
			*((unsigned int *) (ram_array + fpc[i].data_ptr + 12 - ram_start)));

		if (rom_array != NULL && (fpc[i].target_addr - rom_start) < rom_len) {
			memcpy(&rom_array[fpc[i].target_addr - rom_start], &ram_array[fpc[i].data_ptr - ram_start], 16);
		}
	}
}

static void dump_trace() {
	void * buffer[1024];
	const int calls = backtrace(buffer, sizeof(buffer) / sizeof(buffer[0]));
	backtrace_symbols_fd(buffer, calls, 1);
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	signal(SIGSEGV, dump_trace);

	argp_parse(&argp, argc, argv, 0, 0, 0);

	if (!read_file_to_array(ram_file_name, &ram_array, &ram_len)) {
		fprintf(stderr, "ERR: RAM file empty or unavailable.\n");
		exit(EXIT_FAILURE);
	}

	if (rom_file_in_name != NULL && rom_file_out_name != NULL) {
		if (!read_file_to_array(rom_file_in_name, &rom_array, &rom_len)) {
			fprintf(stderr, "ERR: ROM file empty or unavailable.\n");
			exit(EXIT_FAILURE);
		}
	}

	if (bcm43596 == 1)
		analyse_ram_bcm43596();
	else if (bcm4366 == 1)
		analyse_ram_bcm4366();
	else
		analyse_ram();

	if (rom_file_in_name != NULL && rom_file_out_name != NULL) {
		write_array_to_file(rom_file_out_name, rom_array, rom_len);
	}

	exit(EXIT_SUCCESS);
}
