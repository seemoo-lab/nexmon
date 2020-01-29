/*
 * firmware cutter for broadcom 43xx wireless driver files
 * 
 * Copyright (c) 2005 Martin Langer <martin-langer@gmx.de>,
 *               2005-2007 Michael Buesch <m@bues.ch>
 *		 2005 Alex Beregszaszi
 *		 2007 Johannes Berg <johannes@sipsolutions.net>
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     1. Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *     2. Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *   THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 *   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY
 *   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#if defined(__DragonFly__) || defined(__FreeBSD__)
#include <sys/endian.h>
#else
#include <byteswap.h>
#endif

#include "md5.h"
#include "fwcutter.h"
#include "fwcutter_list.h"

#if defined(__DragonFly__) || defined(__FreeBSD__)
#define V3_FW_DIRNAME	"v3"
#define V4_FW_DIRNAME	"v4"
#else
#define V3_FW_DIRNAME	"b43legacy"
#define V4_FW_DIRNAME	"b43"
#endif

static struct cmdline_args cmdargs;


/* check whether file will be listed/extracted from */
static int file_ok(const struct file *f)
{
	return !(f->flags & FW_FLAG_UNSUPPORTED) || cmdargs.unsupported;
}

/* Convert a Big-Endian 16bit integer to CPU-endian */
static uint16_t from_be16(be16_t v)
{
	uint16_t ret = 0;

	ret |= (uint16_t)(((uint8_t *)&v)[0]) << 8;
	ret |= (uint16_t)(((uint8_t *)&v)[1]);

	return ret;
}

/* Convert a CPU-endian 16bit integer to Big-Endian */
static be16_t to_be16(uint16_t v)
{
	return (be16_t)from_be16((be16_t)v);
}

/* Convert a Big-Endian 32bit integer to CPU-endian */
static uint32_t from_be32(be32_t v)
{
	uint32_t ret = 0;

	ret |= (uint32_t)(((uint8_t *)&v)[0]) << 24;
	ret |= (uint32_t)(((uint8_t *)&v)[1]) << 16;
	ret |= (uint32_t)(((uint8_t *)&v)[2]) << 8;
	ret |= (uint32_t)(((uint8_t *)&v)[3]);

	return ret;
}

/* Convert a CPU-endian 32bit integer to Big-Endian */
static be32_t to_be32(uint32_t v)
{
	return (be32_t)from_be32((be32_t)v);
}

/* tiny disassembler */
static void print_ucode_version(struct insn *insn)
{
	int val;

	/*
	 * The instruction we're looking for is a store to memory
	 * offset insn->op3 of the constant formed like `val' below.
	 * 0x2de00 is the opcode for type 1, 0x378 is the opcode
	 * for type 2 and 3.
	 */
	if (insn->opcode != 0x378 && insn->opcode != 0x2de00)
		return;

	val = ((0xFF & insn->op1) << 8) | (0xFF & insn->op2);

	/*
	 * Memory offsets are word-offsets, for the meaning
	 * see http://bcm-v4.sipsolutions.net/802.11/ObjectMemory
	 */
	switch (insn->op3) {
	case 0:
		printf("  ucode version:  %d\n", val);
		break;
	case 1:
		printf("  ucode revision: %d\n", val);
		break;
	case 2:
		printf("  ucode date:     %.4d-%.2d-%.2d\n",
		       2000 + (val >> 12), (val >> 8) & 0xF, val & 0xFF);
		break;
	case 3:
		printf("  ucode time:     %.2d:%.2d:%.2d\n",
		       val >> 11, (val >> 5) & 0x3f, val & 0x1f);
		break;
	}
}

static void disasm_ucode_1(uint64_t in, struct insn *out)
{
	/* xxyyyzzz00oooooX -> ooooo Xxx yyy zzz
	 * if we swap the upper and lower 32-bits first it becomes easier:
	 * 00oooooxxxyyyzzz -> ooooo xxx yyy zzz
	 */
	in = (in >> 32) | (in << 32);

	out->op3	= in & 0xFFF;
	out->op2	= (in >> 12) & 0xFFF;
	out->op1	= (in >> 24) & 0xFFF;
	out->opcode	= (in >> 36) & 0xFFFFF;
	/* the rest of the in word should be zero */
}

static void disasm_ucode_2(uint64_t in, struct insn *out)
{
	/* xxyyyzzz0000oooX -> ooo Xxx yyy zzz
	 * if we swap the upper and lower 32-bits first it becomes easier:
	 * 0000oooxxxyyyzzz -> ooo xxx yyy zzz
	 */
	in = (in >> 32) | (in << 32);

	out->op3	= in & 0xFFF;
	out->op2	= (in >> 12) & 0xFFF;
	out->op1	= (in >> 24) & 0xFFF;
	out->opcode	= (in >> 36) & 0xFFF;
	/* the rest of the in word should be zero */
}

static void disasm_ucode_3(uint64_t in, struct insn *out)
{
	/*
	 * like 2, but each operand has one bit more; appears
	 * to use the same instruction set slightly extended
	 */
	in = (in >> 32) | (in << 32);

	out->op3	= in & 0x1FFF;
	out->op2	= (in >> 13) & 0x1FFF;
	out->op1	= (in >> 26) & 0x1FFF;
	out->opcode	= (in >> 39) & 0xFFF;
	/* the rest of the in word should be zero */
}

static void analyse_ucode(int ucode_rev, uint8_t *data, uint32_t len)
{
	uint64_t *insns = (uint64_t*)data;
	struct insn insn;
	uint32_t i;

	for (i=0; i<len/sizeof(*insns); i++) {
		switch (ucode_rev) {
		case 1:
			disasm_ucode_1(insns[i], &insn);
			print_ucode_version(&insn);
			break;
		case 2:
			disasm_ucode_2(insns[i], &insn);
			print_ucode_version(&insn);
			break;
		case 3:
			disasm_ucode_3(insns[i], &insn);
			print_ucode_version(&insn);
			break;
		}
	}
}

static void swap_endianness_ucode(uint8_t *buf, uint32_t len)
{
	uint32_t *buf32 = (uint32_t*)buf;
	uint32_t i;

	for (i=0; i<len/4; i++)
		buf32[i] = bswap_32(buf32[i]);
}

#define swap_endianness_pcm swap_endianness_ucode

static void swap_endianness_iv(struct iv *iv)
{
	iv->reg = bswap_16(iv->reg);
	iv->size = bswap_16(iv->size);
	iv->val = bswap_32(iv->val);
}

static void build_ivs(struct b43_iv **_out, size_t *_out_size,
		      struct iv *in, size_t in_size,
		      struct fw_header *hdr,
		      uint32_t flags)
{
	struct iv *iv;
	struct b43_iv *out;
	uint32_t i;
	size_t out_size = 0;

	if (sizeof(struct b43_iv) != 6) {
		printf("sizeof(struct b43_iv) != 6\n");
		exit(255);
	}

	out = malloc(in_size);
	if (!out) {
		perror("failed to allocate buffer");
		exit(1);
	}
	*_out = out;
	for (i = 0; i < in_size / sizeof(*iv); i++, in++) {
		if (flags & FW_FLAG_LE)
			swap_endianness_iv(in);
		/* input-IV is BigEndian */
		if (in->reg & to_be16(~FW_IV_OFFSET_MASK)) {
			printf("Input file IV offset > 0x%X\n", FW_IV_OFFSET_MASK);
			exit(1);
		}
		out->offset_size = in->reg;
		if (in->size == to_be16(4)) {
			out->offset_size |= to_be16(FW_IV_32BIT);
			out->data.d32 = in->val;

			out_size += sizeof(be16_t) + sizeof(be32_t);
			out = (struct b43_iv *)((uint8_t *)out + sizeof(be16_t) + sizeof(be32_t));
		} else if (in->size == to_be16(2)) {
			if (in->val & to_be32(~0xFFFF)) {
				printf("Input file 16bit IV value overflow\n");
				exit(1);
			}
			out->data.d16 = to_be16(from_be32(in->val));

			out_size += sizeof(be16_t) + sizeof(be16_t);
			out = (struct b43_iv *)((uint8_t *)out + sizeof(be16_t) + sizeof(be16_t));
		} else {
			printf("Input file IV size != 2|4\n");
			exit(1);
		}
	}
	hdr->size = to_be32(i);
	*_out_size = out_size;
}

static void write_file(const char *name, uint8_t *buf, uint32_t len,
		       const struct fw_header *hdr, uint32_t flags)
{
	FILE *f;
	char nbuf[4096];
	const char *dir;
	int r;

	if (flags & FW_FLAG_V4)
		dir = V4_FW_DIRNAME;
	else
		dir = V3_FW_DIRNAME;

	r = snprintf(nbuf, sizeof(nbuf),
		     "%s/%s", cmdargs.target_dir, dir);
	if (r >= sizeof(nbuf)) {
		fprintf(stderr, "name too long");
		exit(2);
	}

	r = mkdir(nbuf, 0770);
	if (r && errno != EEXIST) {
		perror("failed to create output directory");
		exit(2);
	}

	r = snprintf(nbuf, sizeof(nbuf),
		     "%s/%s/%s.fw", cmdargs.target_dir, dir, name);
	if (r >= sizeof(nbuf)) {
		fprintf(stderr, "name too long");
		exit(2);
	}
	f = fopen(nbuf, "w");
	if (!f) {
		perror("failed to open file");
		exit(2);
	}
	if (fwrite(hdr, sizeof(*hdr), 1, f) != 1) {
		perror("failed to write file");
		exit(2);
	}
	if (fwrite(buf, 1, len, f) != len) {
		perror("failed to write file");
		exit(2);
	}
	fclose(f);
}

static void extract_or_identify(FILE *f, const struct extract *extract,
				uint32_t flags)
{
	uint8_t *buf;
	size_t data_length;
	int ucode_rev = 0;
	struct fw_header hdr;

	memset(&hdr, 0, sizeof(hdr));
	hdr.ver = FW_HDR_VER;

	if (fseek(f, extract->offset, SEEK_SET)) {
		perror("failed to seek on file");
		exit(2);
	}

	buf = malloc(extract->length);
	if (!buf) {
		perror("failed to allocate buffer");
		exit(3);
	}
	if (fread(buf, 1, extract->length, f) != extract->length) {
		perror("failed to read complete data");
		exit(3);
	}

	switch (extract->type) {
	case EXT_UCODE_3:
		ucode_rev += 1;
	case EXT_UCODE_2:
		ucode_rev += 1;
	case EXT_UCODE_1:
		ucode_rev += 1;
		data_length = extract->length;
		if (flags & FW_FLAG_LE)
			swap_endianness_ucode(buf, data_length);
		analyse_ucode(ucode_rev, buf, data_length);
		hdr.type = FW_TYPE_UCODE;
		hdr.size = to_be32(data_length);
		break;
	case EXT_PCM:
		data_length = extract->length;
		if (flags & FW_FLAG_LE)
			swap_endianness_pcm(buf, data_length);
		hdr.type = FW_TYPE_PCM;
		hdr.size = to_be32(data_length);
		break;
	case EXT_IV: {
		struct b43_iv *ivs;

		hdr.type = FW_TYPE_IV;
		build_ivs(&ivs, &data_length,
			  (struct iv *)buf, extract->length,
			  &hdr, flags);
		free(buf);
		buf = (uint8_t *)ivs;
		break;
	}
	default:
		exit(255);
	}

	if (cmdargs.mode == FWCM_EXTRACT)
		write_file(extract->name, buf, data_length, &hdr, flags);

	free(buf);
}

static void print_banner(void)
{
	printf("b43-fwcutter version " FWCUTTER_VERSION "\n");
}

static void print_file(const struct file *file)
{
	char filename[30];
	char shortname[30];

	if (file->flags & FW_FLAG_V4)
		printf(V4_FW_DIRNAME "\t\t");
	else
		printf(V3_FW_DIRNAME "\t");

	if (strlen(file->name) > 20) {
		strncpy(shortname, file->name, 20);
		shortname[20] = '\0';
		snprintf(filename, sizeof(filename), "%s..", shortname);
	} else
		strcpy (filename, file->name);

	printf("%s\t", filename);
	if (strlen(filename) < 8) printf("\t");
	if (strlen(filename) < 16) printf("\t");

	printf("%s\t", file->ucode_version);
	if (strlen(file->ucode_version) < 8) printf("\t");

	printf("%s\n", file->md5);
}

static void print_supported_files(void)
{
	int i;

	print_banner();
	printf("\nExtracting firmware is possible "
	       "from these binary driver files.\n"
	       "Please read http://linuxwireless.org/en/users/Drivers/b43#devicefirmware\n\n");
	printf("<driver>\t"
	       "<filename>\t\t"
	       "<microcode>\t"
	       "<MD5 checksum>\n\n");
	/* print for legacy driver first */
	for (i = 0; i < ARRAY_SIZE(files); i++)
		if (file_ok(&files[i]) && !(files[i].flags & FW_FLAG_V4))
			print_file(&files[i]);
	for (i = 0; i < ARRAY_SIZE(files); i++)
		if (file_ok(&files[i]) && files[i].flags & FW_FLAG_V4)
			print_file(&files[i]);
	printf("\n");
}

static const struct file *find_file(FILE *fd)
{
	unsigned char buffer[16384], signature[16];
	struct MD5Context md5c;
	char md5sig[33];
	int i;

	MD5Init(&md5c);
	while ((i = (int) fread(buffer, 1, sizeof(buffer), fd)) > 0)
		MD5Update(&md5c, buffer, (unsigned) i);
	MD5Final(signature, &md5c);

	snprintf(md5sig, sizeof(md5sig),
		 "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x"
		 "%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x",
		 signature[0], signature[1], signature[2], signature[3],
		 signature[4], signature[5], signature[6], signature[7],
		 signature[8], signature[9], signature[10], signature[11],
		 signature[12], signature[13], signature[14], signature[15]);

	for (i = 0; i < ARRAY_SIZE(files); i++) {
		if (file_ok(&files[i]) &&
		    strcasecmp(md5sig, files[i].md5) == 0) {
			printf("This file is recognised as:\n");
			printf("  filename   :  %s\n", files[i].name);
			printf("  version    :  %s\n", files[i].ucode_version);
			printf("  MD5        :  %s\n", files[i].md5);
			return &files[i];
		}
	}
	printf("Sorry, the input file is either wrong or "
	       "not supported by b43-fwcutter.\n");
	printf("This file has an unknown MD5sum %s.\n", md5sig);

	return NULL;
}

static void print_usage(int argc, char *argv[])
{
	print_banner();
	printf("\nA tool to extract firmware for a Broadcom 43xx device\n");
	printf("from a proprietary Broadcom 43xx device driver file.\n");
	printf("\nUsage: %s [OPTION] [proprietary-driver-file]\n", argv[0]);
	printf("  --unsupported         "
	       "Allow working on extractable but unsupported drivers\n");
	printf("  -l|--list             "
	       "List supported driver versions\n");
	printf("  -i|--identify         "
	       "Only identify the driver file (don't extract)\n");
	printf("  -w|--target-dir DIR   "
	       "Extract and write firmware to DIR\n");
	printf("  -v|--version          "
	       "Print b43-fwcutter version\n");
	printf("  -h|--help             "
	       "Print this help\n");
	printf("\nExample: %s -w /lib/firmware wl_apsta.o\n"
	       "         to extract the firmware blobs from wl_apsta.o and store\n"
	       "         the resulting firmware in /lib/firmware\n",
	       argv[0]);
}

static int do_cmp_arg(char **argv, int *pos,
		      const char *template,
		      int allow_merged,
		      char **param)
{
	char *arg;
	char *next_arg;
	size_t arg_len, template_len;

	arg = argv[*pos];
	next_arg = argv[*pos + 1];
	arg_len = strlen(arg);
	template_len = strlen(template);

	if (param) {
		/* Maybe we have a merged parameter here.
		 * A merged parameter is "-pfoobar" for example.
		 */
		if (allow_merged && arg_len > template_len) {
			if (memcmp(arg, template, template_len) == 0) {
				*param = arg + template_len;
				return ARG_MATCH;
			}
			return ARG_NOMATCH;
		} else if (arg_len != template_len)
			return ARG_NOMATCH;
		*param = next_arg;
	}
	if (strcmp(arg, template) == 0) {
		if (param) {
			/* Skip the parameter on the next iteration. */
			(*pos)++;
			if (!*param) {
				printf("%s needs a parameter\n", arg);
				return ARG_ERROR;
			}
		}
		return ARG_MATCH;
	}

	return ARG_NOMATCH;
}

/* Simple and lean command line argument parsing. */
static int cmp_arg(char **argv, int *pos,
		   const char *long_template,
		   const char *short_template,
		   char **param)
{
	int err;

	if (long_template) {
		err = do_cmp_arg(argv, pos, long_template, 0, param);
		if (err == ARG_MATCH || err == ARG_ERROR)
			return err;
	}
	err = ARG_NOMATCH;
	if (short_template)
		err = do_cmp_arg(argv, pos, short_template, 1, param);
	return err;
}

static int parse_args(int argc, char *argv[])
{
	int i, res;
	char *param;

	if (argc < 2)
		goto out_usage;
	for (i = 1; i < argc; i++) {
		res = cmp_arg(argv, &i, "--list", "-l", NULL);
		if (res == ARG_MATCH) {
			cmdargs.mode = FWCM_LIST;
			continue;
		} else if (res == ARG_ERROR)
			goto out;

		res = cmp_arg(argv, &i, "--version", "-v", NULL);
		if (res == ARG_MATCH) {
			print_banner();
			return 1;
		} else if (res == ARG_ERROR)
			goto out;

		res = cmp_arg(argv, &i, "--help", "-h", NULL);
		if (res == ARG_MATCH)
			goto out_usage;
		else if (res == ARG_ERROR)
			goto out;

		res = cmp_arg(argv, &i, "--identify", "-i", NULL);
		if (res == ARG_MATCH) {
			cmdargs.mode = FWCM_IDENTIFY;
			continue;
		} else if (res == ARG_ERROR)
			goto out;

		res = cmp_arg(argv, &i, "--unsupported", NULL, NULL);
		if (res == ARG_MATCH) {
			cmdargs.unsupported = 1;
			continue;
		} else if (res == ARG_ERROR)
			goto out;

		res = cmp_arg(argv, &i, "--target-dir", "-w", &param);
		if (res == ARG_MATCH) {
			cmdargs.target_dir = param;
			continue;
		} else if (res == ARG_ERROR)
			goto out;

		cmdargs.infile = argv[i];
		break;
	}

	if (!cmdargs.infile && cmdargs.mode != FWCM_LIST)
		goto out_usage;
	return 0;

out_usage:
	print_usage(argc, argv);
out:
	return -1;	
}

int main(int argc, char *argv[])
{
	FILE *fd;
	const struct file *file;
	const struct extract *extract;
	int err;
	const char *dir;

	cmdargs.target_dir = ".";
	err = parse_args(argc, argv);
	if (err == 1)
		return 0;
	else if (err != 0)
		return err;

	if (cmdargs.mode == FWCM_LIST) {
		print_supported_files();
		return 0;
	}

	fd = fopen(cmdargs.infile, "rb");
	if (!fd) {
		fprintf(stderr, "Cannot open input file %s\n", cmdargs.infile);
		return 2;
	}

	err = -1;
	file = find_file(fd);
	if (!file)
		goto out_close;

	if (file->flags & FW_FLAG_V4)
		dir = V4_FW_DIRNAME;
	else
		dir = V3_FW_DIRNAME;

	extract = file->extract;
	while (extract->name) {
		printf("%s %s/%s.fw\n",
		       cmdargs.mode == FWCM_IDENTIFY ? "Contains" : "Extracting",
		       dir, extract->name);
		extract_or_identify(fd, extract, file->flags);
		extract++;
	}

	err = 0;
out_close:
	fclose(fd);

	return err;
}
