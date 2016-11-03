#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
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
long ram_len = 0;

char *ram_file_name = AS_STR(RAM_FILE_NAME);
int ucode_start = 0;
int ucode_length = 0;
char *ucode_file_name = "";

const char *argp_program_version = "fpext";
const char *argp_program_bug_address = "<mschulz@seemoo.tu-darmstadt.de>";

static char doc[] = "fpext -- a program to extract flash patches form a firmware rom.";

static struct argp_option options[] = {
	{"ramfile", 'r', "FILE", 0, "Read ram from FILE"},
	{"ucodestart", 'b', "ADDR", 0, "Start address of ucode"},
	{"ucodelength", 'l', "LEN", 0, "Size of ucode in firmware"},
	{"ucodefile", 'o', "FILE", 0, "Write extracted firmware to FILE"},
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
			ucode_start = strtol(arg, NULL, 0);
			break;

		case 'l':
			ucode_length = strtol(arg, NULL, 0);
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

void
analyse_ram(FILE *fp)
{
	unsigned char *ucode = (unsigned char *) &ram_array[ucode_start];
	int i = 0;
	unsigned int a = 0;
	unsigned int b = 0;

	for (i = 0; i < ucode_length; i+=7) {
		ucode += 7;
		a = (*(ucode - 3) << 16) | (*(ucode - 4) << 24) | *(ucode - 1) | (*(ucode - 2) << 8);
		b = (*(ucode - 6) << 8) | (*(ucode - 7) << 16) | *(ucode - 5);
		fwrite(&a, sizeof(int), 1, fp);
		fwrite(&b, sizeof(int), 1, fp);
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

	FILE *fp;

	if((fp = fopen(ucode_file_name, "wb"))) {
		analyse_ram(fp);
		fclose(fp);
	}

	exit(EXIT_SUCCESS);
}
