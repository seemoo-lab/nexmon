/* dftest.c
 * Shows display filter byte-code, for debugging dfilter routines.
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <errno.h>

#include <glib.h>

#include <epan/epan.h>
#include <epan/timestamp.h>
#include <epan/prefs.h>
#include <epan/dfilter/dfilter.h>

#ifdef HAVE_PLUGINS
#include <wsutil/plugins.h>
#endif
#include <wsutil/filesystem.h>
#include <wsutil/privileges.h>
#include <wsutil/report_err.h>

#include "ui/util.h"
#include "register.h"

static void failure_message(const char *msg_format, va_list ap);
static void open_failure_message(const char *filename, int err,
	gboolean for_writing);
static void read_failure_message(const char *filename, int err);
static void write_failure_message(const char *filename, int err);

int
main(int argc, char **argv)
{
	char		*init_progfile_dir_error;
	char		*text;
	char		*gpf_path, *pf_path;
	int		gpf_open_errno, gpf_read_errno;
	int		pf_open_errno, pf_read_errno;
	dfilter_t	*df;
	gchar		*err_msg;

	/*
	 * Get credential information for later use.
	 */
	init_process_policies();

	/*
	 * Attempt to get the pathname of the executable file.
	 */
	init_progfile_dir_error = init_progfile_dir(argv[0], main);
	if (init_progfile_dir_error != NULL) {
		fprintf(stderr, "dftest: Can't get pathname of dftest program: %s.\n",
			init_progfile_dir_error);
	}

	init_report_err(failure_message, open_failure_message,
			read_failure_message, write_failure_message);

	timestamp_set_type(TS_RELATIVE);
	timestamp_set_seconds_type(TS_SECONDS_DEFAULT);

#ifdef HAVE_PLUGINS
	/* Register all the plugin types we have. */
	epan_register_plugin_types(); /* Types known to libwireshark */

	/* Scan for plugins.  This does *not* call their registration routines;
	   that's done later. */
	scan_plugins();
#endif

	/* Register all dissectors; we must do this before checking for the
	   "-g" flag, as the "-g" flag dumps a list of fields registered
	   by the dissectors, and we must do it before we read the preferences,
	   in case any dissectors register preferences. */
	if (!epan_init(register_all_protocols, register_all_protocol_handoffs,
	    NULL, NULL))
		return 2;

	/* set the c-language locale to the native environment. */
	setlocale(LC_ALL, "");

	read_prefs(&gpf_open_errno, &gpf_read_errno, &gpf_path,
		&pf_open_errno, &pf_read_errno, &pf_path);
	if (gpf_path != NULL) {
		if (gpf_open_errno != 0) {
			fprintf(stderr,
				"can't open global preferences file \"%s\": %s.\n",
				pf_path, g_strerror(gpf_open_errno));
		}
		if (gpf_read_errno != 0) {
			fprintf(stderr,
				"I/O error reading global preferences file \"%s\": %s.\n",
				pf_path, g_strerror(gpf_read_errno));
		}
	}
	if (pf_path != NULL) {
		if (pf_open_errno != 0) {
			fprintf(stderr,
				"can't open your preferences file \"%s\": %s.\n",
				pf_path, g_strerror(pf_open_errno));
		}
		if (pf_read_errno != 0) {
			fprintf(stderr,
				"I/O error reading your preferences file \"%s\": %s.\n",
				pf_path, g_strerror(pf_read_errno));
		}
	}

	/* notify all registered modules that have had any of their preferences
	changed either from one of the preferences file or from the command
	line that its preferences have changed. */
	prefs_apply_all();

	/* Check for filter on command line */
	if (argc <= 1) {
		fprintf(stderr, "Usage: dftest <filter>\n");
		exit(1);
	}

	/* Get filter text */
	text = get_args_as_string(argc, argv, 1);

	printf("Filter: \"%s\"\n", text);

	/* Compile it */
	if (!dfilter_compile(text, &df, &err_msg)) {
		fprintf(stderr, "dftest: %s\n", err_msg);
		g_free(err_msg);
		epan_cleanup();
		exit(2);
	}

	printf("\n");

	if (df == NULL)
		printf("Filter is empty\n");
	else
		dfilter_dump(df);

	dfilter_free(df);
	epan_cleanup();
	exit(0);
}

/*
 * General errors are reported with an console message in "dftest".
 */
static void
failure_message(const char *msg_format, va_list ap)
{
	fprintf(stderr, "dftest: ");
	vfprintf(stderr, msg_format, ap);
	fprintf(stderr, "\n");
}

/*
 * Open/create errors are reported with an console message in "dftest".
 */
static void
open_failure_message(const char *filename, int err, gboolean for_writing)
{
	fprintf(stderr, "dftest: ");
	fprintf(stderr, file_open_error_message(err, for_writing), filename);
	fprintf(stderr, "\n");
}

/*
 * Read errors are reported with an console message in "dftest".
 */
static void
read_failure_message(const char *filename, int err)
{
	fprintf(stderr, "dftest: An error occurred while reading from the file \"%s\": %s.\n",
		filename, g_strerror(err));
}

/*
 * Write errors are reported with an console message in "dftest".
 */
static void
write_failure_message(const char *filename, int err)
{
	fprintf(stderr, "dftest: An error occurred while writing to the file \"%s\": %s.\n",
		filename, g_strerror(err));
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
