#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <argp.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define AS_STR2(TEXT) #TEXT
#define AS_STR(TEXT) AS_STR2(TEXT)

char *ram_array = NULL;
size_t ram_len = 0;

char *ram_file_name = AS_STR(RAM_FILE_NAME);
size_t ucode_start = 0;
size_t ucode_length = 0;
char *ucode_file_name = "";

const char *argp_program_version = "ucodeext";
const char *argp_program_bug_address = "<mschulz@seemoo.tu-darmstadt.de>";

static char doc[] = "ucodeext -- a program to extract ucode from a firmware ram image.";

static struct argp_option options[] = {
	{"ramfile", 'r', "FILE", 0, "Read ram from FILE"},
	{"ucodestart", 'b', "ADDR", 0, "Start address of ucode"},
	{"ucodelength", 'l', "LEN", 0, "Size of ucode in firmware"},
	{"ucodefile", 'o', "FILE", 0, "Write extracted firmware to FILE"},
	{ 0 }
};

static size_t
parse_size_arg(const char *arg, struct argp_state *state)
{
	char *endptr = NULL;
	unsigned long long val;

	if (arg == NULL || *arg == '\0')
		argp_error(state, "Missing numeric argument");

	while (*arg == ' ' || *arg == '\t')
		arg++;
	if (*arg == '-')
		argp_error(state, "Negative offset/length not allowed: '%s'", arg);

	errno = 0;
	val = strtoull(arg, &endptr, 0);
	if (errno != 0 || endptr == arg || *endptr != '\0' || val > (unsigned long long)SIZE_MAX)
		argp_error(state, "Invalid unsigned integer: '%s'", arg);

	return (size_t)val;
}

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
	switch (key) {

		case 'r':
			ram_file_name = arg;
			break;

		case 'b':
			ucode_start = parse_size_arg(arg, state);
			break;

		case 'l':
			ucode_length = parse_size_arg(arg, state);
			break;

		case 'o':
			ucode_file_name = arg;
			break;
		
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };

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

int
analyse_ram(FILE *fp)
{
	const unsigned char *base = (const unsigned char *)ram_array + ucode_start;

	if (ucode_length % 7 != 0) {
		fprintf(stderr, "WARN: ucode length %zu is not a multiple of 7 bytes; ignoring %zu trailing byte(s).\n",
			ucode_length, ucode_length % 7);
	}

	for (size_t i = 0; i + 7 <= ucode_length; i += 7) {
		const unsigned char *p = base + i;
		uint32_t a = ((uint32_t)p[3] << 24) | ((uint32_t)p[4] << 16) | ((uint32_t)p[5] << 8) | (uint32_t)p[6];
		uint32_t b = ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | (uint32_t)p[2];
		if (fwrite(&a, sizeof(uint32_t), 1, fp) != 1 ||
		    fwrite(&b, sizeof(uint32_t), 1, fp) != 1) {
			return -1;
		}
	}

	return 0;
}

int
main(int argc, char **argv)
{
	argp_parse(&argp, argc, argv, 0, 0, 0);

	if (ucode_file_name == NULL || ucode_file_name[0] == '\0') {
		fprintf(stderr, "ERR: output ucode file (-o) not specified.\n");
		exit(EXIT_FAILURE);
	}

	if (!read_file_to_array(ram_file_name, &ram_array, &ram_len)) {
		fprintf(stderr, "ERR: ram file empty or unavailable.\n");
		exit(EXIT_FAILURE);
	}

	if (ucode_length == 0 || ucode_start > ram_len || ucode_length > ram_len - ucode_start) {
		fprintf(stderr, "ERR: ucode range [0x%zx, 0x%zx) out of RAM bounds [0x0, 0x%zx).\n",
			ucode_start, ucode_start + ucode_length, ram_len);
		free(ram_array);
		exit(EXIT_FAILURE);
	}

	FILE *fp = fopen(ucode_file_name, "wb");
	if (!fp) {
		fprintf(stderr, "ERR: unable to open output ucode file '%s'.\n", ucode_file_name);
		free(ram_array);
		exit(EXIT_FAILURE);
	}

	if (analyse_ram(fp) != 0 || fclose(fp) != 0) {
		fprintf(stderr, "ERR: failed writing extracted ucode to '%s'.\n", ucode_file_name);
		free(ram_array);
		exit(EXIT_FAILURE);
	}

	free(ram_array);
	exit(EXIT_SUCCESS);
}
