#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <argp.h>
#include <string.h>
#include <dirent.h>

#include <hci.h>

FILE* hcdfile = NULL;

char *out_file_name = NULL;
char *indir_name = NULL;

const char *argp_program_version = "bthcd_generate";
const char *argp_program_bug_address = "<https://github.com/seemoo-lab/nexmon>";

static char doc[] = "bthcd_generate -- Bluetooth HCD-File Generator";

static struct argp_option options[] = {
	{"hcdfile", 'o', "FILE", 0, "Output HCD file name"},
	{"indir", 'i', "FILE", 0, "Input directory with the extracted patches"},
	{ 0 }
};

static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
	switch (key) {

		case 'o':
			out_file_name = arg;
			break;
		
		case 'i':
			indir_name = arg;
			break;
		
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = { options, parse_opt, 0, doc };

// read_file_to_array
// params:
// *filename: name of the file to read.
// **buffer:	buffer to read the file content to.
// *filelen:	length of the file read to buffer.
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
		if(fileptr) {
			fileptr = NULL;
		}

		return *filelen;
	}
	printf("Error: Could not open file %s to write to", filename);
	return 0;
}

// process_patch_section
// params:
// *section_file_name: name of the file containing the next patch section
// addr:							 address the patch in thi section should be applied to
//
int
process_patch_section(char* section_file_name, uint32_t addr)
{
	char* section_array = NULL;
	long section_len = 0;
	int write_index = 0;
	uint8_t len = 0;
	uint8_t hci_len = 0;

	printf("Processing file %s [0x%08X] ...\n", section_file_name, addr);

	if(!read_file_to_array(section_file_name, &section_array, &section_len)) {
		fprintf(stderr, "ERR: section file empty or unavailable.\n");
		exit(EXIT_FAILURE);
	}

	// We're only able to write a maximum of 251 bytes payload using the WRITE_RAM
	// HCI Command. Therefore we need to check wether the section is bigger than 
	// 251 bytes. It this is the case we need to split the section into multiple 
	// WRITE_RAM commands.
	while(write_index < section_len) {
		len = (section_len - write_index) <= 251 ? (section_len - write_index) : 251;
		hci_len = len + sizeof(addr);
		// HCI-Command: WRITE_RAM (0xFC_4C)
		// WRITE_RAM <addr> <data>
		fwrite(HCI_WRITE_RAM_STR, 2, 1, hcdfile);
		fwrite(&hci_len, sizeof(hci_len), 1, hcdfile);
		fwrite(&addr, sizeof(addr), 1, hcdfile);
		fwrite(&section_array[write_index], len, 1, hcdfile);
		write_index += len;
		addr += len;
	}

	if(section_array){
		free(section_array);
		section_array = NULL;
	}
	return 1;
}

int
main(int argc, char **argv)
{
	struct dirent **file_list;
	char filename[1024];
	char addr_str[11];
	uint32_t addr;
	int i, n;

	argp_parse(&argp, argc, argv, 0, 0, 0);

	if(out_file_name == NULL) {
		fprintf(stderr, "ERR: Output file name (HCD file) not specified.\n");
		exit(EXIT_FAILURE);
	}

	if(indir_name == NULL) {
		fprintf(stderr, "ERR: Input directory name not specified.\n");
		exit(EXIT_FAILURE);
	}

	hcdfile = fopen(out_file_name,"wb");

	if ((n = scandir (indir_name, &file_list, 0, alphasort)) < 0) {
		fprintf(stderr, "ERR: cannot open input directory.\n");
		exit(EXIT_FAILURE);
	}

	for(i = 0; i < n; i++) {
		// filename must look like this: 00_writeram_0x<addr>.bin
		if(strncmp(file_list[i]->d_name + 3, "writeram_", 9) == 0){
			snprintf(filename, sizeof(filename), "%s/%s", indir_name, file_list[i]->d_name);
			strncpy(addr_str, file_list[i]->d_name + 12, 10);
			addr = (uint32_t) strtol(addr_str, NULL, 0);
			process_patch_section(filename, addr);
		}
		if(file_list[i]){
			free(file_list[i]);
			file_list[i] = NULL;
		}
	}
	if(file_list) {
		free(file_list);
		file_list = NULL;	
	}
	// HCD Command: LAUNCH_RAM (0xFC_4E)
	// LAUNCH_RAM 0x04 0xff_ff_ff_ff 
	// To issue the bluetooth-chip to reboot into the normal bluetooth mode
	fwrite(HCI_LAUNCH_RAM_STR_NEXUS_5, 7, 1, hcdfile);
	fclose(hcdfile);
	if(hcdfile){
		hcdfile = NULL;
	}

	exit(EXIT_SUCCESS);
}
