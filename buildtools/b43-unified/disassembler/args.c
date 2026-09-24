/*
 *   Copyright (C) 2006-2007  Michael Buesch <m@bues.ch>
 *   Copyright (C) 2024 Francesco Gringoli <francesco.gringoli@unibs.it>
 *   Copyright (C) 2024 Jakob Link <jlink@seemoo.de>
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

arch_t arch_supported [] = {ARCH5, ARCH15, ARCH65, ARCH129, ARCH132, ARCH134, ARCHINVALID};

subarch_valid_t subarch_supported [] = {
	{ARCH129, SUBARCH1 },
	{ARCH134, SUBARCH1 },
	{ARCHINVALID, SUBARCHINVALID},
};

struct cmdline_args cmdargs = {
	.debug			= 0,
	.arch			= (int) ARCH5,
	.subarch		= (int) NOSUBARCH,
	.informat		= FMT_B43,
	.print_addresses	= 0,
	.unknown_decode		= 0,
	.suppress_warnings	= 0,
	.emit_weird		= 0,
};

#define ARG_MATCH		0
#define ARG_NOMATCH		1
#define ARG_ERROR		-1

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

static void usage(FILE *fd, int argc, char **argv)
{
	fprintf(fd, "Usage: %s INPUT_FILE OUTPUT_FILE [OPTIONS]\n", argv[0]);
	fprintf(fd, "  -a|--arch ARCH      The architecture type of the input file\n");
	fprintf(fd, "  -f|--format FMT     Input file format. FMT must be one of:\n");
	fprintf(fd, "                      raw-le32, raw-be32, b43\n");
	fprintf(fd, "  -p|--paddr          Print the code addresses\n");
	fprintf(fd, "  -u|--unkdec         Decode operands of unknown instructions\n");
	fprintf(fd, "  -d|--debug          Print verbose debugging info\n");
	fprintf(fd, "                      Repeat for more verbose debugging\n");
	fprintf(fd, "  -s|--subarch SUBAR  The sub-architecture of the input file\n");
	fprintf(fd, "  -e|--weird          Emit weird bit info (requires arch >= 129,\n");
	fprintf(fd, "                      does not work for arch 129, subarch 1)\n");
	fprintf(fd, "  -n|--nowarn         Suppress_warnings\n");
	fprintf(fd, "  -h|--help           Print this help\n");
}

int parse_args(int argc, char **argv)
{
	int i;
	int res;
	char *param;

	infile_name = NULL;
	outfile_name = NULL;

	for (i = 1; i < argc; i++) {
		if ((res = cmp_arg(argv, &i, "--help", "-h", NULL)) == ARG_MATCH) {
			usage(stdout, argc, argv);
			return 1;
		} else if ((res = cmp_arg(argv, &i, "--format", "-f", &param)) == ARG_MATCH) {
			if (strcasecmp(param, "raw-le32") == 0)
				cmdargs.informat = FMT_RAW_LE32;
			else if (strcasecmp(param, "raw-be32") == 0)
				cmdargs.informat = FMT_RAW_BE32;
			else if (strcasecmp(param, "b43") == 0)
				cmdargs.informat = FMT_B43;
			else {
				fprintf(stderr, "Invalid -f|--format\n");
				return -1;
			}
		} else if ((res = cmp_arg(argv, &i, "--paddr", "-p", NULL)) == ARG_MATCH) {
			cmdargs.print_addresses = 1;
		} else if ((res = cmp_arg(argv, &i, "--unkdec", "-u", NULL)) == ARG_MATCH) {
			cmdargs.unknown_decode = 1;
		} else if ((res = cmp_arg(argv, &i, "--debug", "-d", NULL)) == ARG_MATCH) {
			cmdargs.debug++;
		} else if ((res = cmp_arg(argv, &i, "--weird", "-e", NULL)) == ARG_MATCH) {
			cmdargs.emit_weird = 1;
		} else if ((res = cmp_arg(argv, &i, "--nowarn", "-n", NULL)) == ARG_MATCH) {
			cmdargs.suppress_warnings = 1;
		} else if ((res = cmp_arg(argv, &i, "--arch", "-a", &param)) == ARG_MATCH) {
			unsigned long arch;
			char *tail;

			arch = strtol(param, &tail, 0);

			if (strlen(tail)) {
				fprintf (stderr, "Invalid arch parameter\n");
				return -1;
			}

			int kk;
			int supported_arch_number = sizeof (arch_supported) / sizeof (arch_t);
			for (kk = 0; arch_supported [kk] != ARCHINVALID; kk ++)
			{
				if (arch == arch_supported [kk])
					break;
			}
			if (arch_supported [kk] == ARCHINVALID) {
				fprintf(stderr, "Unsupported architecture\n");
				fprintf(stderr, "Only these archs are supported: ");
				for (kk = 0; kk < supported_arch_number - 1; kk ++) {
					fprintf(stderr, "%d ", arch_supported [kk]);
				}
				fprintf (stderr, "\n");
				return -1;
			}
			cmdargs.arch = arch;
		} else if ((res == cmp_arg(argv, &i, "--subarch", "-s", &param)) == ARG_MATCH) {
			unsigned long subarch;
			char *tail;

			subarch = strtol(param, &tail, 0);

			if (strlen(tail)) {
				fprintf (stderr, "Invalid subarch parameter\n");
				return -1;
			}

			cmdargs.subarch = subarch;
		} else {
			if (!infile_name) {
				infile_name = argv[i];
				continue;
			}
			if (!outfile_name) {
				outfile_name = argv[i];
				continue;
			}
			fprintf(stderr, "Unrecognized argument: %s\n", argv[i]);
			goto out_usage;
		}
	}


	/* check if subarch exists */
	if (cmdargs.subarch != NOSUBARCH) {
		int kk;
		for (kk = 0; subarch_supported [kk].arch != ARCHINVALID; kk ++) {
			if (cmdargs.arch == subarch_supported [kk].arch &&
			    cmdargs.subarch == subarch_supported [kk].subarch)
				break;
		}
		if (subarch_supported [kk].arch == ARCHINVALID) {
			fprintf(stderr, "Unsupported subarchitecture, ");
			fprintf(stderr, "only these subarchs are supported:\n");
			for (kk = 0; subarch_supported [kk].arch != ARCHINVALID; kk ++) {
				fprintf(stderr, "\tarch %d subarch %d\n",
					subarch_supported [kk].arch,
					subarch_supported [kk].subarch);
			}
			return -1;
		}
	}
	if (cmdargs.emit_weird && cmdargs.arch < ARCH129)
		goto out_usage;
	if (cmdargs.emit_weird && cmdargs.arch == ARCH129 && cmdargs.subarch == SUBARCH1)
		goto out_usage;
	if (!infile_name || !outfile_name)
		goto out_usage;

	return 0;
out_usage:
	usage(stderr, argc, argv);
	return -1;
}

int open_input_file(void)
{
	if (strcmp(infile_name, "-") == 0) {
		infile = stdin;
	} else {
		infile = fopen(infile_name, "r");
		if (!infile) {
			fprintf(stderr, "Could not open INPUT_FILE %s\n",
				infile_name);
			return -1;
		}
	}

	return 0;
}

void close_input_file(void)
{
	if (strcmp(infile_name, "-") != 0)
		fclose(infile);
}

int open_output_file(void)
{
	if (strcmp(outfile_name, "-") == 0) {
		outfile = stdout;
	} else {
		outfile = fopen(outfile_name, "w+");
		if (!outfile) {
			fprintf(stderr, "Could not open OUTPUT_FILE %s\n",
				outfile_name);
			return -1;
		}
	}

	return 0;
}

void close_output_file(void)
{
	if (strcmp(outfile_name, "-") != 0)
		fclose(outfile);
}
