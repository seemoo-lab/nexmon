#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
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
size_t ram_len = 0;

char *rom_array = NULL;
size_t rom_len = 0;

char *ram_file_name = AS_STR(RAM_FILE_NAME);
char *rom_file_in_name = NULL;
char *rom_file_out_name = NULL;
uint32_t fp_config_base = 0;
uint32_t fp_config_end = 0;
uint32_t ram_start = 0x180000;
uint32_t rom_start = 0x0;
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

static uint32_t
parse_u32_arg(const char *arg, struct argp_state *state)
{
	char *endptr = NULL;
	unsigned long val;

	if (arg == NULL || *arg == '\0')
		argp_error(state, "Missing numeric argument");

	while (*arg == ' ' || *arg == '\t')
		arg++;
	if (*arg == '-')
		argp_error(state, "Negative address/offset not allowed: '%s'", arg);

	errno = 0;
	val = strtoul(arg, &endptr, 0);
	if (errno != 0 || endptr == arg || *endptr != '\0' || val > UINT32_MAX)
		argp_error(state, "Invalid 32-bit unsigned integer: '%s'", arg);

	return (uint32_t)val;
}

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
	switch (key) {

		case 'r':
			ram_file_name = arg;
			break;

		case 'b':
			fp_config_base = parse_u32_arg(arg, state);
			break;

		case 'e':
			fp_config_end = parse_u32_arg(arg, state);
			break;

		case 's':
			ram_start = parse_u32_arg(arg, state);
			break;

		case 'i':
			rom_file_in_name = arg;
			break;

		case 'o':
			rom_file_out_name = arg;
			break;

		case 't':
			rom_start = parse_u32_arg(arg, state);
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
write_array_to_file(const char *filename, const char *buffer, size_t filelen)
{
	FILE *fileptr;

	if (filename == NULL || filename[0] == '\0' || buffer == NULL)
		return -1;

	if ((fileptr = fopen(filename, "wb"))) {
		size_t written = fwrite(buffer, 1, filelen, fileptr);
		if (fclose(fileptr) != 0 || written != filelen)
			return -1;
		return 0;
	}

	return -1;
}

int
read_file_to_array(const char *filename, char **buffer, size_t *filelen)
{
	FILE *fileptr;

	if (filename == NULL || filename[0] == '\0')
		return 0;

	if ((fileptr = fopen(filename, "rb"))) {
		long len;
		if (fseek(fileptr, 0, SEEK_END) != 0) {
			fclose(fileptr);
			return 0;
		}
		len = ftell(fileptr);
		if (len <= 0) {
			fclose(fileptr);
			return 0;
		}
		rewind(fileptr);

		*buffer = (char *) calloc((size_t)len + 1, 1);
		if (*buffer == NULL) {
			fclose(fileptr);
			return 0;
		}
		if (fread(*buffer, 1, (size_t)len, fileptr) != (size_t)len) {
			free(*buffer);
			*buffer = NULL;
			fclose(fileptr);
			return 0;
		}
		fclose(fileptr);

		*filelen = (size_t)len;
		return 1;
	}

	return 0;
}

static int
ram_range_valid(uint32_t addr, size_t len)
{
	size_t off;
	if (ram_array == NULL || addr < ram_start)
		return 0;
	off = (size_t)(addr - ram_start);
	if (off > ram_len || len > ram_len - off)
		return 0;
	return 1;
}

static int
rom_range_valid(uint32_t addr, size_t len)
{
	size_t off;
	if (rom_array == NULL || addr < rom_start)
		return 0;
	off = (size_t)(addr - rom_start);
	if (off > rom_len || len > rom_len - off)
		return 0;
	return 1;
}

int
get_words(uint32_t addr, uint16_t *low, uint16_t *high)
{
	if (ram_range_valid(addr, 4)) {
		size_t off = (size_t)(addr - ram_start);
		memcpy(low, ram_array + off, sizeof(*low));
		memcpy(high, ram_array + off + 2, sizeof(*high));
		return 2;
	}

	return 0;
}

struct fp_config {
	uint32_t target_addr;
	uint32_t size;
	uint32_t data_ptr;
};

void
analyse_ram(void)
{
	darm_t d;
	darm_t *dd = &d;
	uint16_t low = 0, high = 0;
	size_t count = (size_t)(fp_config_end - fp_config_base) / sizeof(struct fp_config);
	struct fp_config *fpc = (struct fp_config *) (ram_array + (fp_config_base - ram_start));

	darm_init(&d);

	for (size_t i = 0; i < count; i++) {
		size_t need_ram = fpc[i].size < 4 ? 4 : (size_t)fpc[i].size;
		if (!ram_range_valid(fpc[i].data_ptr, need_ram) ||
		    get_words(fpc[i].data_ptr, &low, &high) != 2) {
			fprintf(stderr, "ERR: Flash patch %zu data_ptr 0x%08x (size %u) out of RAM bounds.\n",
				i, fpc[i].data_ptr, fpc[i].size);
			exit(EXIT_FAILURE);
		}
		memset(&d, 0, sizeof(d));
		darm_disasm(dd, low, high, 1);

		printf("__attribute__((weak))\n");
		printf("__attribute__((at(0x%08x, \"flashpatch\")))\n", fpc[i].target_addr);
		printf("BPatchAddr(flash_patch_%zu, 0x%08x);\n\n", i, (uint32_t)(fpc[i].target_addr + dd->imm + 4));

		if (rom_array != NULL) {
			if (rom_range_valid(fpc[i].target_addr, (size_t)fpc[i].size)) {
				memcpy(&rom_array[fpc[i].target_addr - rom_start],
				       &ram_array[fpc[i].data_ptr - ram_start],
				       (size_t)fpc[i].size);
			}
		}
	}
}

struct fp_config_bcm43596 {
	uint32_t target_addr;
	uint32_t data_ptr;
};

void
analyse_ram_bcm43596(void)
{
	darm_t d;
	darm_t *dd = &d;
	uint16_t low = 0, high = 0;
	size_t count = (size_t)(fp_config_end - fp_config_base) / sizeof(struct fp_config_bcm43596);
	struct fp_config_bcm43596 *fpc = (struct fp_config_bcm43596 *) (ram_array + (fp_config_base - ram_start));

	darm_init(&d);

	for (size_t i = 0; i < count; i++) {
		uint32_t w0, w1;
		size_t off;
		if (!ram_range_valid(fpc[i].data_ptr, 8) ||
		    get_words(fpc[i].data_ptr, &low, &high) != 2) {
			fprintf(stderr, "ERR: Flash patch %zu data_ptr 0x%08x (size 8) out of RAM bounds.\n",
				i, fpc[i].data_ptr);
			exit(EXIT_FAILURE);
		}
		memset(&d, 0, sizeof(d));
		darm_disasm(dd, low, high, 1);

		off = (size_t)(fpc[i].data_ptr - ram_start);
		memcpy(&w0, ram_array + off, sizeof(w0));
		memcpy(&w1, ram_array + off + 4, sizeof(w1));

		printf("__attribute__((weak))\n");
		printf("__attribute__((at(0x%08x, \"flashpatch\")))\n", fpc[i].target_addr);
		printf("unsigned int flash_patch_%zu[2] = {0x%08x, 0x%08x};\n\n", i, w0, w1);

		if (rom_array != NULL) {
			if (rom_range_valid(fpc[i].target_addr, 8)) {
				memcpy(&rom_array[fpc[i].target_addr - rom_start],
				       &ram_array[off], 8);
			}
		}
	}
}

void
analyse_ram_bcm4366(void)
{
	darm_t d;
	darm_t *dd = &d;
	uint16_t low = 0, high = 0;
	size_t count = (size_t)(fp_config_end - fp_config_base) / sizeof(struct fp_config_bcm43596);
	struct fp_config_bcm43596 *fpc = (struct fp_config_bcm43596 *) (ram_array + (fp_config_base - ram_start));

	darm_init(&d);

	for (size_t i = 0; i < count; i++) {
		uint32_t w0, w1, w2, w3;
		size_t off;
		if (!ram_range_valid(fpc[i].data_ptr, 16) ||
		    get_words(fpc[i].data_ptr, &low, &high) != 2) {
			fprintf(stderr, "ERR: Flash patch %zu data_ptr 0x%08x (size 16) out of RAM bounds.\n",
				i, fpc[i].data_ptr);
			exit(EXIT_FAILURE);
		}
		memset(&d, 0, sizeof(d));
		darm_disasm(dd, low, high, 1);

		off = (size_t)(fpc[i].data_ptr - ram_start);
		memcpy(&w0, ram_array + off, sizeof(w0));
		memcpy(&w1, ram_array + off + 4, sizeof(w1));
		memcpy(&w2, ram_array + off + 8, sizeof(w2));
		memcpy(&w3, ram_array + off + 12, sizeof(w3));

		printf("__attribute__((weak))\n");
		printf("__attribute__((at(0x%08x, \"flashpatch\")))\n", fpc[i].target_addr);
		printf("unsigned int flash_patch_%zu[4] = {0x%08x, 0x%08x, 0x%08x, 0x%08x};\n\n", i, w0, w1, w2, w3);

		if (rom_array != NULL) {
			if (rom_range_valid(fpc[i].target_addr, 16)) {
				memcpy(&rom_array[fpc[i].target_addr - rom_start],
				       &ram_array[off], 16);
			}
		}
	}
}

static void dump_trace(int sig) {
	void * buffer[1024];
	const int calls = backtrace(buffer, sizeof(buffer) / sizeof(buffer[0]));
	(void)sig;
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

	if (fp_config_end < fp_config_base ||
	    !ram_range_valid(fp_config_base, (size_t)(fp_config_end - fp_config_base))) {
		fprintf(stderr, "ERR: Flash patch config range [0x%08x, 0x%08x) out of RAM bounds [0x%08x, 0x%08zx).\n",
			fp_config_base, fp_config_end, ram_start, (size_t)ram_start + ram_len);
		free(ram_array);
		exit(EXIT_FAILURE);
	}

	if (rom_file_in_name != NULL && rom_file_out_name != NULL) {
		if (!read_file_to_array(rom_file_in_name, &rom_array, &rom_len)) {
			fprintf(stderr, "ERR: ROM file empty or unavailable.\n");
			free(ram_array);
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
		if (write_array_to_file(rom_file_out_name, rom_array, rom_len) != 0) {
			fprintf(stderr, "ERR: Failed to write patched ROM file '%s'.\n", rom_file_out_name);
			free(rom_array);
			free(ram_array);
			exit(EXIT_FAILURE);
		}
	}

	free(rom_array);
	free(ram_array);
	exit(EXIT_SUCCESS);
}
