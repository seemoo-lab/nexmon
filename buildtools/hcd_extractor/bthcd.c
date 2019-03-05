#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <argp.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <bluetooth/hci.h>

#define AS_STR2(TEXT) #TEXT
#define AS_STR(TEXT) AS_STR2(TEXT)

char *patch_array = NULL;
long patch_len = 0;

char *patch_file_name = AS_STR(RAM_FILE_NAME);
char *firmware_file_name = 0;
char *outdir_name = 0;

const char *argp_program_version = "bthcd";
const char *argp_program_bug_address = "https://github.com/seemoo-lab/nexmon";

static char doc[] = "hcd_extract -- a program to extract hcd patches from a hcd patch file.";

static struct argp_option options[] = {
	{"hcdfile", 'p', "FILE", 0, "Read firmware patch file from FILE"},
	{"firmware", 'f', "FILE", 0, "Firmware that will receive the patches"},
	{"outdir", 'o', "FILE", 0, "Output directory for the extracted patches"},
	{ 0 }
};

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
	switch (key) {

		case 'p':
			patch_file_name = arg;
			break;

		case 'f':
			firmware_file_name = arg;
			break;
		
		case 'o':
			outdir_name = arg;
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

// analyze_patch_file
// params: void
// Analyzes the given patch.
// Primarly the WriteRAM patches are searched. If a WriteRAM patch is found it is written into a
// file with the naming syntax:
// 10_writeram_0x<targetaddress>.bin
// In addition to the single extracted WriteRAM patches a patchout_complete.bin file is created.
// This file contains all patches at their respective	target addresses.
void
analyze_patch_file(void)
{
	int i = 0;
	uint32_t addrcnt = 0;
	bool addrcnt_new = true;
	FILE *patch_out_file = NULL;
	char patch_out_file_name[256];
	char *whole_patch;
	long whole_patch_len;

	
	if(firmware_file_name) {
		read_file_to_array(firmware_file_name, &whole_patch, &whole_patch_len);
	} else {
		whole_patch = (char *) malloc(0x240000);
		whole_patch_len = 0x240000;
	}

	
	while (i < patch_len) {
		// Each WriteRAM patch consists of the layout:
		// < 2-byte: cmd > < 1 byte: length of patch content > < 4 byte patch target >
		uint16_t cmd = *(uint16_t *) (void *) &patch_array[i];
		uint8_t len = *(uint8_t *) (void *) &patch_array[i+2];
		uint32_t addr = *(uint32_t *) (void *) &patch_array[i+3];

		if (cmd != HCI_WRITE_RAM) {
			printf("Found cmd which is not WriteRAM. Ignore it: %04x\n", cmd);
			i += sizeof(cmd) + sizeof(len) + len;
			continue;
		}

		if (addrcnt_new || addrcnt != addr) {
			addrcnt_new = false;
			printf("new patch section addrcnt - addr: 0x%08x\n", addrcnt - addr);
			snprintf(patch_out_file_name, sizeof(patch_out_file_name), "%s/10_writeram_0x%08x.bin", outdir_name, addr);
			if(patch_out_file){
				fclose(patch_out_file);
				patch_out_file = NULL;
			}
			patch_out_file = fopen(patch_out_file_name,"wb");
		}
		addrcnt = addr + len - sizeof(addr);

		if ((long)addr + len > whole_patch_len) {
			fprintf(stderr, "ERR: patch address out of bounds: 0x%08X\n", addr);
			exit(EXIT_FAILURE);
		}
		memcpy(&whole_patch[addr], &patch_array[i + sizeof(cmd) + sizeof(len) + sizeof(addr)], len - sizeof(addr));

		if (patch_out_file){
			fwrite(&patch_array[i + sizeof(cmd) + sizeof(len) + sizeof(addr)], len - sizeof(addr), 1, patch_out_file);
		}

		printf("cmd %04x len %d addr %08x\n", cmd, len, addr);
		i += sizeof(cmd) + sizeof(len) + len;
	}

	patch_out_file = fopen("patchout_complete.bin","wb");
	if (patch_out_file){
		fwrite(whole_patch, whole_patch_len, 1, patch_out_file);
		free(patch_out_file);
		patch_out_file = NULL;
	}
	if (whole_patch){
		free(whole_patch);
		whole_patch = NULL;
	}
}

// main
// params (see top of this file for more information about the params):
// f (optional): 
// p (mandatory):
// o (optional):
// Starts a full analysis of the given HCD-patch file
int
main(int argc, char **argv)
{
	argp_parse(&argp, argc, argv, 0, 0, 0);

	if(!read_file_to_array(patch_file_name, &patch_array, &patch_len)) {
		fprintf(stderr, "ERR: patch file empty or unavailable.\n");
		exit(EXIT_FAILURE);
	}

	if(outdir_name == NULL) {
		outdir_name = ".";
	} else {
		if(strlen(outdir_name) > 200) {
			outdir_name[200] = '\0';
		}

		struct stat st = {0};
		if (stat(outdir_name, &st) == -1) {
			mkdir(outdir_name, 0700);
		}
	}

	analyze_patch_file();

	if (patch_array) {
		free(patch_array);
		patch_array = NULL;
	};

	exit(EXIT_SUCCESS);
}
