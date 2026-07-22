/*
 *   Copyright (C) 2006-2010  Michael Buesch <m@bues.ch>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2
 *   as published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#include "args.h"
#include "main.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>


struct cmdline_args cmdargs = {
	.debug			= 0,
	.print_sizes		= 0,
	.initvals_fn_extension	= ".initvals",
	.outformat		= FMT_B43,
	.rawivals		= 0,
};


#define ARG_MATCH		0
#define ARG_NOMATCH		1
#define ARG_ERROR		-1

static int do_cmp_arg(char **argv, int *pos,
		      const char *template,
		      int allow_merged,
		      const char **param)
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
			if (*param == NULL) {
				fprintf(stderr, "%s needs a parameter\n", arg);
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
		   const char **param)
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

static void usage(void)
{
	printf("Usage: b43-asm INPUT_FILE OUTPUT_FILE [OPTIONS]\n");
	printf("  -f|--format FMT     Output file format. FMT must be one of:\n");
	printf("                      raw-le32, raw-be32, b43\n");
	printf("  -d|--debug          Print verbose debugging info\n");
	printf("                      Repeat for more verbose debugging\n");
	printf("  -s|--psize          Print the size of the code after assembling\n");
	printf("  -e|--ivalext EXT    Filename extension for the initvals\n");
	printf("  -r|--rawivals       Use new Broadcom format (wl)\n");
	printf("  -h|--help           Print this help\n");
}

int parse_args(int argc, char **argv)
{
	int i;
	int res;
	const char *param;

	if (argc < 3)
		goto out_usage;
	infile_name = argv[1];
	outfile_name = argv[2];

	for (i = 3; i < argc; i++) {
		if ((res = cmp_arg(argv, &i, "--help", "-h", NULL)) == ARG_MATCH) {
			usage();
			return 1;
		} else if ((res = cmp_arg(argv, &i, "--format", "-f", &param)) == ARG_MATCH) {
			if (strcasecmp(param, "raw-le32") == 0)
				cmdargs.outformat = FMT_RAW_LE32;
			else if (strcasecmp(param, "raw-be32") == 0)
				cmdargs.outformat = FMT_RAW_BE32;
			else if (strcasecmp(param, "b43") == 0)
				cmdargs.outformat = FMT_B43;
			else {
				fprintf(stderr, "Invalid -f|--format\n\n");
				goto out_usage;
			}
		} else if ((res = cmp_arg(argv, &i, "--debug", "-d", NULL)) == ARG_MATCH) {
			cmdargs.debug++;
		} else if ((res = cmp_arg(argv, &i, "--psize", "-s", NULL)) == ARG_MATCH) {
			cmdargs.print_sizes = 1;
		} else if ((res = cmp_arg(argv, &i, "--ivalext", "-e", &param)) == ARG_MATCH) {
			cmdargs.initvals_fn_extension = param;
		} else if ((res = cmp_arg(argv, &i, "--__real_infile", NULL, &param)) == ARG_MATCH) {
			cmdargs.real_infile_name = param;
		} else if ((res = cmp_arg(argv, &i, "--rawivals", "-r", NULL)) == ARG_MATCH) {
			cmdargs.rawivals = 1;
		} else {
			fprintf(stderr, "Unrecognized argument: %s\n", argv[i]);
			goto out_usage;
		}
	}
	if (!cmdargs.real_infile_name)
		cmdargs.real_infile_name = infile_name;
	if (strcmp(cmdargs.real_infile_name, outfile_name) == 0) {
		fprintf(stderr, "Error: INPUT and OUTPUT filename must not be the same\n");
		goto out_usage;
	}

	return 0;
out_usage:
	usage();
	return -1;
}

int open_input_file(void)
{
	int fd;
	int err;

	if (strcmp(infile_name, "-") == 0) {
		/* infile == stdin */
		fd = STDIN_FILENO;
	} else {
		fd = open(infile_name, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "Could not open INPUT_FILE %s\n",
				infile_name);
			return -1;
		}
		err = dup2(fd, STDIN_FILENO);
		if (err) {
			fprintf(stderr, "Could not dup INPUT_FILE %s "
				"to STDIN\n", infile_name);
			close(fd);
			return -1;
		}
	}
	infile.fd = fd;

	return 0;
}

void close_input_file(void)
{
	if (strcmp(infile_name, "-") != 0)
		close(infile.fd);
}
