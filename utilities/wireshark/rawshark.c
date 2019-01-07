/* rawshark.c
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Rawshark - Raw field extractor by Gerald Combs <gerald@wireshark.org>
 * and Loris Degioanni <loris.degioanni@cacetech.com>
 * Based on TShark, by Gilbert Ramirez <gram@alumni.rice.edu> and Guy Harris
 * <guy@alum.mit.edu>.
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

/*
 * Rawshark does the following:
 * - Opens a specified file or named pipe
 * - Applies a specfied DLT or "decode as" encapsulation
 * - Reads frames prepended with a libpcap packet header.
 * - Prints a status line, followed by fields from a specified list.
 */

#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <limits.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <errno.h>

#ifndef HAVE_GETOPT_LONG
#include "wsutil/wsgetopt.h"
#endif

#include <glib.h>
#include <epan/epan-int.h>
#include <epan/epan.h>

#include <wsutil/cmdarg_err.h>
#include <wsutil/crash_info.h>
#include <wsutil/filesystem.h>
#include <wsutil/file_util.h>
#include <wsutil/plugins.h>
#include <wsutil/privileges.h>
#include <wsutil/report_err.h>

#include "globals.h"
#include <epan/packet.h>
#include <epan/ftypes/ftypes-int.h>
#include "file.h"
#include "frame_tvbuff.h"
#include <epan/disabled_protos.h>
#include <epan/prefs.h>
#include <epan/column.h>
#include <epan/print.h>
#include <epan/addr_resolv.h>
#ifdef HAVE_LIBPCAP
#include "ui/capture_ui_utils.h"
#endif
#include "ui/util.h"
#include "register.h"
#include "conditions.h"
#include "capture_stop_conditions.h"
#include <epan/epan_dissect.h>
#include <epan/stat_tap_ui.h>
#include <epan/timestamp.h>
#include <wsutil/unicode-utils.h>
#include "epan/column-utils.h"
#include "epan/proto.h"
#include <epan/tap.h>

#include <wiretap/wtap.h>
#include <wiretap/libpcap.h>
#include <wiretap/pcap-encap.h>

#include <wsutil/clopts_common.h>
#include <ws_version_info.h>

#include "caputils/capture-pcap-util.h"

#ifdef HAVE_EXTCAP
#include "extcap.h"
#endif

#ifdef HAVE_LIBPCAP
#include <setjmp.h>
#ifdef _WIN32
#include "caputils/capture-wpcap.h"
#endif /* _WIN32 */
#endif /* HAVE_LIBPCAP */
#include "log.h"

#if 0
/*
 * This is the template for the decode as option; it is shared between the
 * various functions that output the usage for this parameter.
 */
static const gchar decode_as_arg_template[] = "<layer_type>==<selector>,<decode_as_protocol>";
#endif

static guint32 cum_bytes;
static const frame_data *ref;
static frame_data ref_frame;
static frame_data *prev_dis;
static frame_data prev_dis_frame;
static frame_data *prev_cap;
static frame_data prev_cap_frame;

/*
 * The way the packet decode is to be written.
 */
typedef enum {
    WRITE_TEXT, /* summary or detail text */
    WRITE_XML   /* PDML or PSML */
    /* Add CSV and the like here */
} output_action_e;

static gboolean line_buffered;
static print_format_e print_format = PR_FMT_TEXT;

static gboolean want_pcap_pkthdr;

cf_status_t raw_cf_open(capture_file *cf, const char *fname);
static gboolean load_cap_file(capture_file *cf);
static gboolean process_packet(capture_file *cf, epan_dissect_t *edt, gint64 offset,
                               struct wtap_pkthdr *whdr, const guchar *pd);
static void show_print_file_io_error(int err);

static void open_failure_message(const char *filename, int err,
                                 gboolean for_writing);
static void failure_message(const char *msg_format, va_list ap);
static void read_failure_message(const char *filename, int err);
static void write_failure_message(const char *filename, int err);
static void rawshark_cmdarg_err(const char *fmt, va_list ap);
static void rawshark_cmdarg_err_cont(const char *fmt, va_list ap);
static void protocolinfo_init(char *field);
static gboolean parse_field_string_format(char *format);

typedef enum {
    SF_NONE,    /* No format (placeholder) */
    SF_NAME,    /* %D Field name / description */
    SF_NUMVAL,  /* %N Numeric value */
    SF_STRVAL   /* %S String value */
} string_fmt_e;

typedef struct string_fmt_s {
    gchar *plain;
    string_fmt_e format;    /* Valid if plain is NULL */
} string_fmt_t;

capture_file cfile;
int n_rfilters;
int n_rfcodes;
dfilter_t *rfcodes[64];
int n_rfieldfilters;
dfilter_t *rfieldfcodes[64];
int fd;
int encap;
GPtrArray *string_fmts;

static void
print_usage(FILE *output)
{
    fprintf(output, "\n");
    fprintf(output, "Usage: rawshark [options] ...\n");
    fprintf(output, "\n");

    fprintf(output, "Input file:\n");
    fprintf(output, "  -r <infile>              set the pipe or file name to read from\n");

    fprintf(output, "\n");
    fprintf(output, "Processing:\n");
    fprintf(output, "  -d <encap:linktype>|<proto:protoname>\n");
    fprintf(output, "                           packet encapsulation or protocol\n");
    fprintf(output, "  -F <field>               field to display\n");
    fprintf(output, "  -n                       disable all name resolution (def: all enabled)\n");
    fprintf(output, "  -N <name resolve flags>  enable specific name resolution(s): \"mnNtd\"\n");
    fprintf(output, "  -p                       use the system's packet header format\n");
    fprintf(output, "                           (which may have 64-bit timestamps)\n");
    fprintf(output, "  -R <read filter>         packet filter in Wireshark display filter syntax\n");
    fprintf(output, "  -s                       skip PCAP header on input\n");

    fprintf(output, "\n");
    fprintf(output, "Output:\n");
    fprintf(output, "  -l                       flush output after each packet\n");
    fprintf(output, "  -S                       format string for fields\n");
    fprintf(output, "                           (%%D - name, %%S - stringval, %%N numval)\n");
    fprintf(output, "  -t ad|a|r|d|dd|e         output format of time stamps (def: r: rel. to first)\n");

    fprintf(output, "\n");
    fprintf(output, "Miscellaneous:\n");
    fprintf(output, "  -h                       display this help and exit\n");
    fprintf(output, "  -o <name>:<value> ...    override preference setting\n");
    fprintf(output, "  -v                       display version info and exit\n");
}

static void
log_func_ignore (const gchar *log_domain _U_, GLogLevelFlags log_level _U_,
                 const gchar *message _U_, gpointer user_data _U_)
{
}

/**
 * Open a pipe for raw input.  This is a stripped-down version of
 * pcap_loop.c:cap_pipe_open_live().
 * We check if "pipe_name" is "-" (stdin) or a FIFO, and open it.
 * @param pipe_name The name of the pipe or FIFO.
 * @return A POSIX file descriptor on success, or -1 on failure.
 */
static int
raw_pipe_open(const char *pipe_name)
{
#ifndef _WIN32
    ws_statb64 pipe_stat;
#else
    char *pncopy, *pos = NULL;
    DWORD err;
    wchar_t *err_str;
    HANDLE hPipe = NULL;
#endif
    int          rfd;

    g_log(LOG_DOMAIN_CAPTURE_CHILD, G_LOG_LEVEL_DEBUG, "open_raw_pipe: %s", pipe_name);

    /*
     * XXX Rawshark blocks until we return
     */
    if (strcmp(pipe_name, "-") == 0) {
        rfd = 0; /* read from stdin */
#ifdef _WIN32
        /*
         * This is needed to set the stdin pipe into binary mode, otherwise
         * CR/LF are mangled...
         */
        _setmode(0, _O_BINARY);
#endif  /* _WIN32 */
    } else {
#ifndef _WIN32
        if (ws_stat64(pipe_name, &pipe_stat) < 0) {
            fprintf(stderr, "rawshark: The pipe %s could not be checked: %s\n",
                    pipe_name, g_strerror(errno));
            return -1;
        }
        if (! S_ISFIFO(pipe_stat.st_mode)) {
            if (S_ISCHR(pipe_stat.st_mode)) {
                /*
                 * Assume the user specified an interface on a system where
                 * interfaces are in /dev.  Pretend we haven't seen it.
                 */
            } else
            {
                fprintf(stderr, "rawshark: \"%s\" is neither an interface nor a pipe\n",
                        pipe_name);
            }
            return -1;
        }
        rfd = ws_open(pipe_name, O_RDONLY | O_NONBLOCK, 0000 /* no creation so don't matter */);
        if (rfd == -1) {
            fprintf(stderr, "rawshark: \"%s\" could not be opened: %s\n",
                    pipe_name, g_strerror(errno));
            return -1;
        }
#else /* _WIN32 */
#define PIPE_STR "\\pipe\\"
        /* Under Windows, named pipes _must_ have the form
         * "\\<server>\pipe\<pipe_name>".  <server> may be "." for localhost.
         */
        pncopy = g_strdup(pipe_name);
        if (strstr(pncopy, "\\\\") == pncopy) {
            pos = strchr(pncopy + 3, '\\');
            if (pos && g_ascii_strncasecmp(pos, PIPE_STR, strlen(PIPE_STR)) != 0)
                pos = NULL;
        }

        g_free(pncopy);

        if (!pos) {
            fprintf(stderr, "rawshark: \"%s\" is neither an interface nor a pipe\n",
                    pipe_name);
            return -1;
        }

        /* Wait for the pipe to appear */
        while (1) {
            hPipe = CreateFile(utf_8to16(pipe_name), GENERIC_READ, 0, NULL,
                               OPEN_EXISTING, 0, NULL);

            if (hPipe != INVALID_HANDLE_VALUE)
                break;

            err = GetLastError();
            if (err != ERROR_PIPE_BUSY) {
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL, err, 0, (LPTSTR) &err_str, 0, NULL);
                fprintf(stderr, "rawshark: \"%s\" could not be opened: %s (error %lu)\n",
                        pipe_name, utf_16to8(err_str), err);
                LocalFree(err_str);
                return -1;
            }

            if (!WaitNamedPipe(utf_8to16(pipe_name), 30 * 1000)) {
                err = GetLastError();
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL, err, 0, (LPTSTR) &err_str, 0, NULL);
                fprintf(stderr, "rawshark: \"%s\" could not be waited for: %s (error %lu)\n",
                        pipe_name, utf_16to8(err_str), err);
                LocalFree(err_str);
                return -1;
            }
        }

        rfd = _open_osfhandle((intptr_t) hPipe, _O_RDONLY);
        if (rfd == -1) {
            fprintf(stderr, "rawshark: \"%s\" could not be opened: %s\n",
                    pipe_name, g_strerror(errno));
            return -1;
        }
#endif /* _WIN32 */
    }

    return rfd;
}

/**
 * Parse a link-type argument of the form "encap:<pcap linktype>" or
 * "proto:<proto name>".  "Pcap linktype" must be a name conforming to
 * pcap_datalink_name_to_val() or an integer; the integer should be
 * a LINKTYPE_ value supported by Wiretap.  "Proto name" must be
 * a protocol name, e.g. "http".
 */
static gboolean
set_link_type(const char *lt_arg) {
    char *spec_ptr = strchr(lt_arg, ':');
    char *p;
    int dlt_val;
    long val;
    dissector_handle_t dhandle;
    GString *pref_str;

    if (!spec_ptr)
        return FALSE;

    spec_ptr++;

    if (strncmp(lt_arg, "encap:", strlen("encap:")) == 0) {
        dlt_val = linktype_name_to_val(spec_ptr);
        if (dlt_val == -1) {
            errno = 0;
            val = strtol(spec_ptr, &p, 10);
            if (p == spec_ptr || *p != '\0' || errno != 0 || val > INT_MAX) {
                return FALSE;
            }
            dlt_val = (int)val;
        }
        /*
         * In those cases where a given link-layer header type
         * has different LINKTYPE_ and DLT_ values, linktype_name_to_val()
         * will return the OS's DLT_ value for that link-layer header
         * type, not its OS-independent LINKTYPE_ value.
         *
         * On a given OS, wtap_pcap_encap_to_wtap_encap() should
         * be able to map either LINKTYPE_ values or DLT_ values
         * for the OS to the appropriate Wiretap encapsulation.
         */
        encap = wtap_pcap_encap_to_wtap_encap(dlt_val);
        if (encap == WTAP_ENCAP_UNKNOWN) {
            return FALSE;
        }
        return TRUE;
    } else if (strncmp(lt_arg, "proto:", strlen("proto:")) == 0) {
        dhandle = find_dissector(spec_ptr);
        if (dhandle) {
            encap = WTAP_ENCAP_USER0;
            pref_str = g_string_new("uat:user_dlts:");
            /* This must match the format used in the user_dlts file */
            g_string_append_printf(pref_str,
                                   "\"User 0 (DLT=147)\",\"%s\",\"0\",\"\",\"0\",\"\"",
                                   spec_ptr);
            if (prefs_set_pref(pref_str->str) != PREFS_SET_OK) {
                g_string_free(pref_str, TRUE);
                return FALSE;
            }
            g_string_free(pref_str, TRUE);
            return TRUE;
        }
    }
    return FALSE;
}

int
main(int argc, char *argv[])
{
    GString             *comp_info_str;
    GString             *runtime_info_str;
    char                *init_progfile_dir_error;
    int                  opt, i;
    gboolean             arg_error = FALSE;

#ifdef _WIN32
    WSADATA              wsaData;
#endif  /* _WIN32 */

    char                *gpf_path, *pf_path;
    char                *gdp_path, *dp_path;
    int                  gpf_open_errno, gpf_read_errno;
    int                  pf_open_errno, pf_read_errno;
    int                  gdp_open_errno, gdp_read_errno;
    int                  dp_open_errno, dp_read_errno;
    gchar               *pipe_name = NULL;
    gchar               *rfilters[64];
    e_prefs             *prefs_p;
    char                 badopt;
    int                  log_flags;
    GPtrArray           *disp_fields = g_ptr_array_new();
    guint                fc;
    gboolean             skip_pcap_header = FALSE;
    static const struct option long_options[] = {
      {"help", no_argument, NULL, 'h'},
      {"version", no_argument, NULL, 'v'},
      {0, 0, 0, 0 }
    };

#define OPTSTRING_INIT "d:F:hlnN:o:pr:R:sS:t:v"

    static const char    optstring[] = OPTSTRING_INIT;

    /* Set the C-language locale to the native environment. */
    setlocale(LC_ALL, "");

    cmdarg_err_init(rawshark_cmdarg_err, rawshark_cmdarg_err_cont);

    /* Get the compile-time version information string */
    comp_info_str = get_compiled_version_info(NULL, epan_get_compiled_version_info);

    /* Get the run-time version information string */
    runtime_info_str = get_runtime_version_info(NULL);

    /* Add it to the information to be reported on a crash. */
    ws_add_crash_info("Rawshark (Wireshark) %s\n"
           "\n"
           "%s"
           "\n"
           "%s",
        get_ws_vcs_version_info(), comp_info_str->str, runtime_info_str->str);

#ifdef _WIN32
    arg_list_utf_16to8(argc, argv);
    create_app_running_mutex();
#endif /* _WIN32 */

    /*
     * Get credential information for later use.
     */
    init_process_policies();

    /*
     * Clear the filters arrays
     */
    memset(rfilters, 0, sizeof(rfilters));
    memset(rfcodes, 0, sizeof(rfcodes));
    n_rfilters = 0;
    n_rfcodes = 0;

    /*
     * Initialize our string format
     */
    string_fmts = g_ptr_array_new();

    /*
     * Attempt to get the pathname of the executable file.
     */
    init_progfile_dir_error = init_progfile_dir(argv[0], main);
    if (init_progfile_dir_error != NULL) {
        fprintf(stderr, "rawshark: Can't get pathname of rawshark program: %s.\n",
                init_progfile_dir_error);
    }

    /* nothing more than the standard GLib handler, but without a warning */
    log_flags =
        G_LOG_LEVEL_WARNING |
        G_LOG_LEVEL_MESSAGE |
        G_LOG_LEVEL_INFO |
        G_LOG_LEVEL_DEBUG;

    g_log_set_handler(NULL,
                      (GLogLevelFlags)log_flags,
                      log_func_ignore, NULL /* user_data */);
    g_log_set_handler(LOG_DOMAIN_CAPTURE_CHILD,
                      (GLogLevelFlags)log_flags,
                      log_func_ignore, NULL /* user_data */);

    init_report_err(failure_message, open_failure_message, read_failure_message,
                    write_failure_message);

    timestamp_set_type(TS_RELATIVE);
    timestamp_set_precision(TS_PREC_AUTO);
    timestamp_set_seconds_type(TS_SECONDS_DEFAULT);

    /* Register all dissectors; we must do this before checking for the
       "-G" flag, as the "-G" flag dumps information registered by the
       dissectors, and we must do it before we read the preferences, in
       case any dissectors register preferences. */
    if (!epan_init(register_all_protocols, register_all_protocol_handoffs,
                   NULL, NULL))
        return 2;

    prefs_p = read_prefs(&gpf_open_errno, &gpf_read_errno, &gpf_path,
                         &pf_open_errno, &pf_read_errno, &pf_path);
    if (gpf_path != NULL) {
        if (gpf_open_errno != 0) {
            cmdarg_err("Can't open global preferences file \"%s\": %s.",
                       pf_path, g_strerror(gpf_open_errno));
        }
        if (gpf_read_errno != 0) {
            cmdarg_err("I/O error reading global preferences file \"%s\": %s.",
                       pf_path, g_strerror(gpf_read_errno));
        }
    }
    if (pf_path != NULL) {
        if (pf_open_errno != 0) {
            cmdarg_err("Can't open your preferences file \"%s\": %s.", pf_path,
                       g_strerror(pf_open_errno));
        }
        if (pf_read_errno != 0) {
            cmdarg_err("I/O error reading your preferences file \"%s\": %s.",
                       pf_path, g_strerror(pf_read_errno));
        }
        g_free(pf_path);
        pf_path = NULL;
    }

    /* Read the disabled protocols file. */
    read_disabled_protos_list(&gdp_path, &gdp_open_errno, &gdp_read_errno,
                              &dp_path, &dp_open_errno, &dp_read_errno);
    read_disabled_heur_dissector_list(&gdp_path, &gdp_open_errno, &gdp_read_errno,
                              &dp_path, &dp_open_errno, &dp_read_errno);
    if (gdp_path != NULL) {
        if (gdp_open_errno != 0) {
            cmdarg_err("Could not open global disabled protocols file\n\"%s\": %s.",
                       gdp_path, g_strerror(gdp_open_errno));
        }
        if (gdp_read_errno != 0) {
            cmdarg_err("I/O error reading global disabled protocols file\n\"%s\": %s.",
                       gdp_path, g_strerror(gdp_read_errno));
        }
        g_free(gdp_path);
    }
    if (dp_path != NULL) {
        if (dp_open_errno != 0) {
            cmdarg_err(
                "Could not open your disabled protocols file\n\"%s\": %s.", dp_path,
                g_strerror(dp_open_errno));
        }
        if (dp_read_errno != 0) {
            cmdarg_err(
                "I/O error reading your disabled protocols file\n\"%s\": %s.", dp_path,
                g_strerror(dp_read_errno));
        }
        g_free(dp_path);
    }

#ifdef _WIN32
    ws_init_dll_search_path();
    /* Load Wpcap, if possible */
    load_wpcap();
#endif

    cap_file_init(&cfile);

    /* Print format defaults to this. */
    print_format = PR_FMT_TEXT;

    /* Initialize our encapsulation type */
    encap = WTAP_ENCAP_UNKNOWN;

    /* Now get our args */
    /* XXX - We should probably have an option to dump libpcap link types */
    while ((opt = getopt_long(argc, argv, optstring, long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':        /* Payload type */
                if (!set_link_type(optarg)) {
                    cmdarg_err("Invalid link type or protocol \"%s\"", optarg);
                    exit(1);
                }
                break;
            case 'F':        /* Read field to display */
                g_ptr_array_add(disp_fields, g_strdup(optarg));
                break;
            case 'h':        /* Print help and exit */
                printf("Rawshark (Wireshark) %s\n"
                       "Dump and analyze network traffic.\n"
                       "See https://www.wireshark.org for more information.\n",
                       get_ws_vcs_version_info());
                print_usage(stdout);
                exit(0);
                break;
            case 'l':        /* "Line-buffer" standard output */
                /* This isn't line-buffering, strictly speaking, it's just
                   flushing the standard output after the information for
                   each packet is printed; however, that should be good
                   enough for all the purposes to which "-l" is put (and
                   is probably actually better for "-V", as it does fewer
                   writes).

                   See the comment in "process_packet()" for an explanation of
                   why we do that, and why we don't just use "setvbuf()" to
                   make the standard output line-buffered (short version: in
                   Windows, "line-buffered" is the same as "fully-buffered",
                   and the output buffer is only flushed when it fills up). */
                line_buffered = TRUE;
                break;
            case 'n':        /* No name resolution */
                disable_name_resolution();
                break;
            case 'N':        /* Select what types of addresses/port #s to resolve */
                badopt = string_to_name_resolve(optarg, &gbl_resolv_flags);
                if (badopt != '\0') {
                    cmdarg_err("-N specifies unknown resolving option '%c'; valid options are 'd', m', 'n', 'N', and 't'",
                               badopt);
                    exit(1);
                }
                break;
            case 'o':        /* Override preference from command line */
                switch (prefs_set_pref(optarg)) {

                    case PREFS_SET_OK:
                        break;

                    case PREFS_SET_SYNTAX_ERR:
                        cmdarg_err("Invalid -o flag \"%s\"", optarg);
                        exit(1);
                        break;

                    case PREFS_SET_NO_SUCH_PREF:
                    case PREFS_SET_OBSOLETE:
                        cmdarg_err("-o flag \"%s\" specifies unknown preference", optarg);
                        exit(1);
                        break;
                }
                break;
            case 'p':        /* Expect pcap_pkthdr packet headers, which may have 64-bit timestamps */
                want_pcap_pkthdr = TRUE;
                break;
            case 'r':        /* Read capture file xxx */
                pipe_name = g_strdup(optarg);
                break;
            case 'R':        /* Read file filter */
                if(n_rfilters < (int) sizeof(rfilters) / (int) sizeof(rfilters[0])) {
                    rfilters[n_rfilters++] = optarg;
                }
                else {
                    cmdarg_err("Too many display filters");
                    exit(1);
                }
                break;
            case 's':        /* Skip PCAP header */
                skip_pcap_header = TRUE;
                break;
            case 'S':        /* Print string representations */
                if (!parse_field_string_format(optarg)) {
                    cmdarg_err("Invalid field string format");
                    exit(1);
                }
                break;
            case 't':        /* Time stamp type */
                if (strcmp(optarg, "r") == 0)
                    timestamp_set_type(TS_RELATIVE);
                else if (strcmp(optarg, "a") == 0)
                    timestamp_set_type(TS_ABSOLUTE);
                else if (strcmp(optarg, "ad") == 0)
                    timestamp_set_type(TS_ABSOLUTE_WITH_YMD);
                else if (strcmp(optarg, "adoy") == 0)
                    timestamp_set_type(TS_ABSOLUTE_WITH_YDOY);
                else if (strcmp(optarg, "d") == 0)
                    timestamp_set_type(TS_DELTA);
                else if (strcmp(optarg, "dd") == 0)
                    timestamp_set_type(TS_DELTA_DIS);
                else if (strcmp(optarg, "e") == 0)
                    timestamp_set_type(TS_EPOCH);
                else if (strcmp(optarg, "u") == 0)
                    timestamp_set_type(TS_UTC);
                else if (strcmp(optarg, "ud") == 0)
                    timestamp_set_type(TS_UTC_WITH_YMD);
                else if (strcmp(optarg, "udoy") == 0)
                    timestamp_set_type(TS_UTC_WITH_YDOY);
                else {
                    cmdarg_err("Invalid time stamp type \"%s\"",
                               optarg);
                    cmdarg_err_cont(
"It must be \"a\" for absolute, \"ad\" for absolute with YYYY-MM-DD date,");
                    cmdarg_err_cont(
"\"adoy\" for absolute with YYYY/DOY date, \"d\" for delta,");
                    cmdarg_err_cont(
"\"dd\" for delta displayed, \"e\" for epoch, \"r\" for relative,");
                    cmdarg_err_cont(
"\"u\" for absolute UTC, \"ud\" for absolute UTC with YYYY-MM-DD date,");
                    cmdarg_err_cont(
"or \"udoy\" for absolute UTC with YYYY/DOY date.");
                    exit(1);
                }
                break;
            case 'v':        /* Show version and exit */
            {
                show_version("Rawshark (Wireshark)", comp_info_str, runtime_info_str);
                g_string_free(comp_info_str, TRUE);
                g_string_free(runtime_info_str, TRUE);
                exit(0);
                break;
            }
            default:
            case '?':        /* Bad flag - print usage message */
                print_usage(stderr);
            exit(1);
            break;
        }
    }

    /* Notify all registered modules that have had any of their preferences
       changed either from one of the preferences file or from the command
       line that their preferences have changed.
       Initialize preferences before display filters, otherwise modules
       like MATE won't work. */
    prefs_apply_all();

    /* Initialize our display fields */
    for (fc = 0; fc < disp_fields->len; fc++) {
        protocolinfo_init((char *)g_ptr_array_index(disp_fields, fc));
    }
    g_ptr_array_free(disp_fields, TRUE);
    printf("\n");
    fflush(stdout);

    /* If no capture filter or read filter has been specified, and there are
       still command-line arguments, treat them as the tokens of a capture
       filter (if no "-r" flag was specified) or a read filter (if a "-r"
       flag was specified. */
    if (optind < argc) {
        if (pipe_name != NULL) {
            if (n_rfilters != 0) {
                cmdarg_err("Read filters were specified both with \"-R\" "
                           "and with additional command-line arguments");
                exit(1);
            }
            rfilters[n_rfilters] = get_args_as_string(argc, argv, optind);
        }
    }

    /* Make sure we got a dissector handle for our payload. */
    if (encap == WTAP_ENCAP_UNKNOWN) {
        cmdarg_err("No valid payload dissector specified.");
        exit(1);
    }

    if (arg_error) {
        print_usage(stderr);
        exit(1);
    }


#ifdef _WIN32
    /* Start windows sockets */
    WSAStartup( MAKEWORD( 1, 1 ), &wsaData );
#endif /* _WIN32 */

    /* At this point MATE will have registered its field array so we can
       have a tap filter with one of MATE's late-registered fields as part
       of the filter.  We can now process all the "-z" arguments. */
    start_requested_stats();

    /* disabled protocols as per configuration file */
    if (gdp_path == NULL && dp_path == NULL) {
        set_disabled_protos_list();
        set_disabled_heur_dissector_list();
    }

    /* Build the column format array */
    build_column_format_array(&cfile.cinfo, prefs_p->num_cols, TRUE);

    if (n_rfilters != 0) {
        for (i = 0; i < n_rfilters; i++) {
            gchar *err_msg;

            if (!dfilter_compile(rfilters[i], &rfcodes[n_rfcodes], &err_msg)) {
                cmdarg_err("%s", err_msg);
                g_free(err_msg);
                epan_free(cfile.epan);
                epan_cleanup();
#ifdef HAVE_EXTCAP
                extcap_cleanup();
#endif
                exit(2);
            }
            n_rfcodes++;
        }
    }

    if (pipe_name) {
        /*
         * We're reading a pipe (or capture file).
         */

        /*
         * Immediately relinquish any special privileges we have; we must not
         * be allowed to read any capture files the user running Rawshark
         * can't open.
         */
        relinquish_special_privs_perm();

        if (raw_cf_open(&cfile, pipe_name) != CF_OK) {
            epan_free(cfile.epan);
            epan_cleanup();
#ifdef HAVE_EXTCAP
            extcap_cleanup();
#endif
            exit(2);
        }

        /* Do we need to PCAP header and magic? */
        if (skip_pcap_header) {
            size_t bytes_left = sizeof(struct pcap_hdr) + sizeof(guint32);
            gchar buf[sizeof(struct pcap_hdr) + sizeof(guint32)];
            while (bytes_left != 0) {
                ssize_t bytes = ws_read(fd, buf, (int)bytes_left);
                if (bytes <= 0) {
                    cmdarg_err("Not enough bytes for pcap header.");
                    exit(2);
                }
                bytes_left -= bytes;
            }
        }

        /* Process the packets in the file */
        if (!load_cap_file(&cfile)) {
            epan_free(cfile.epan);
            epan_cleanup();
#ifdef HAVE_EXTCAP
            extcap_cleanup();
#endif
            exit(2);
        }
    } else {
        /* If you want to capture live packets, use TShark. */
        cmdarg_err("Input file or pipe name not specified.");
        exit(2);
    }

    epan_free(cfile.epan);
    epan_cleanup();
#ifdef HAVE_EXTCAP
    extcap_cleanup();
#endif
    return 0;
}

/**
 * Read data from a raw pipe.  The "raw" data consists of a libpcap
 * packet header followed by the payload.
 * @param pd [IN] A POSIX file descriptor.  Because that's _exactly_ the sort
 *           of thing you want to use in Windows.
 * @param phdr [OUT] Packet header information.
 * @param err [OUT] Error indicator.  Uses wiretap values.
 * @param err_info [OUT] Error message.
 * @param data_offset [OUT] data offset in the pipe.
 * @return TRUE on success, FALSE on failure.
 */
static gboolean
raw_pipe_read(struct wtap_pkthdr *phdr, guchar * pd, int *err, gchar **err_info, gint64 *data_offset) {
    struct pcap_pkthdr mem_hdr;
    struct pcaprec_hdr disk_hdr;
    ssize_t bytes_read = 0;
    size_t bytes_needed = sizeof(disk_hdr);
    guchar *ptr = (guchar*) &disk_hdr;

    if (want_pcap_pkthdr) {
        bytes_needed = sizeof(mem_hdr);
        ptr = (guchar*) &mem_hdr;
    }

    /* Copied from capture_loop.c */
    while (bytes_needed > 0) {
        bytes_read = ws_read(fd, ptr, (int)bytes_needed);
        if (bytes_read == 0) {
            *err = 0;
            *err_info = NULL;
            return FALSE;
        } else if (bytes_read < 0) {
            *err = errno;
            *err_info = NULL;
            return FALSE;
        }
        bytes_needed -= bytes_read;
        *data_offset += bytes_read;
        ptr += bytes_read;
    }

    if (want_pcap_pkthdr) {
        phdr->ts.secs = mem_hdr.ts.tv_sec;
        phdr->ts.nsecs = (gint32)mem_hdr.ts.tv_usec * 1000;
        phdr->caplen = mem_hdr.caplen;
        phdr->len = mem_hdr.len;
    } else {
        phdr->ts.secs = disk_hdr.ts_sec;
        phdr->ts.nsecs = disk_hdr.ts_usec * 1000;
        phdr->caplen = disk_hdr.incl_len;
        phdr->len = disk_hdr.orig_len;
    }
    bytes_needed = phdr->caplen;

    phdr->pkt_encap = encap;

#if 0
    printf("mem_hdr: %lu disk_hdr: %lu\n", sizeof(mem_hdr), sizeof(disk_hdr));
    printf("tv_sec: %u (%04x)\n", (unsigned int) phdr->ts.secs, (unsigned int) phdr->ts.secs);
    printf("tv_nsec: %d (%04x)\n", phdr->ts.nsecs, phdr->ts.nsecs);
    printf("caplen: %d (%04x)\n", phdr->caplen, phdr->caplen);
    printf("len: %d (%04x)\n", phdr->len, phdr->len);
#endif
    if (bytes_needed > WTAP_MAX_PACKET_SIZE) {
        *err = WTAP_ERR_BAD_FILE;
        *err_info = g_strdup_printf("Bad packet length: %lu\n",
                   (unsigned long) bytes_needed);
        return FALSE;
    }

    ptr = pd;
    while (bytes_needed > 0) {
        bytes_read = ws_read(fd, ptr, (int)bytes_needed);
        if (bytes_read == 0) {
            *err = WTAP_ERR_SHORT_READ;
            *err_info = NULL;
            return FALSE;
        } else if (bytes_read < 0) {
            *err = errno;
            *err_info = NULL;
            return FALSE;
        }
        bytes_needed -= bytes_read;
        *data_offset += bytes_read;
        ptr += bytes_read;
    }
    return TRUE;
}

static gboolean
load_cap_file(capture_file *cf)
{
    int          err;
    gchar       *err_info;
    gint64       data_offset = 0;

    guchar pd[WTAP_MAX_PACKET_SIZE];
    struct wtap_pkthdr phdr;
    epan_dissect_t edt;

    wtap_phdr_init(&phdr);

    epan_dissect_init(&edt, cf->epan, TRUE, FALSE);

    while (raw_pipe_read(&phdr, pd, &err, &err_info, &data_offset)) {
        process_packet(cf, &edt, data_offset, &phdr, pd);
    }

    epan_dissect_cleanup(&edt);

    wtap_phdr_cleanup(&phdr);

    if (err != 0) {
        /* Print a message noting that the read failed somewhere along the line. */
        switch (err) {

            case WTAP_ERR_UNSUPPORTED:
                cmdarg_err("The file \"%s\" contains record data that Rawshark doesn't support.\n(%s)",
                           cf->filename,
                           err_info != NULL ? err_info : "no information supplied");
                g_free(err_info);
                break;

            case WTAP_ERR_SHORT_READ:
                cmdarg_err("The file \"%s\" appears to have been cut short in the middle of a packet.",
                           cf->filename);
                break;

            case WTAP_ERR_BAD_FILE:
                cmdarg_err("The file \"%s\" appears to be damaged or corrupt.\n(%s)",
                           cf->filename,
                           err_info != NULL ? err_info : "no information supplied");
                g_free(err_info);
                break;

            case WTAP_ERR_DECOMPRESS:
                cmdarg_err("The compressed file \"%s\" appears to be damaged or corrupt.\n(%s)",
                           cf->filename,
                           err_info != NULL ? err_info : "no information supplied");
                g_free(err_info);
                break;

            default:
                cmdarg_err("An error occurred while reading the file \"%s\": %s.",
                           cf->filename, wtap_strerror(err));
                break;
        }
        return FALSE;
    }

    return TRUE;
}

static gboolean
process_packet(capture_file *cf, epan_dissect_t *edt, gint64 offset,
               struct wtap_pkthdr *whdr, const guchar *pd)
{
    frame_data fdata;
    gboolean passed;
    int i;

    if(whdr->len == 0)
    {
        /* The user sends an empty packet when he wants to get output from us even if we don't currently have
           packets to process. We spit out a line with the timestamp and the text "void"
        */
        printf("%lu %lu %lu void -\n", (unsigned long int)cf->count,
               (unsigned long int)whdr->ts.secs,
               (unsigned long int)whdr->ts.nsecs);

        fflush(stdout);

        return FALSE;
    }

    /* Count this packet. */
    cf->count++;

    /* If we're going to print packet information, or we're going to
       run a read filter, or we're going to process taps, set up to
       do a dissection and do so. */
    frame_data_init(&fdata, cf->count, whdr, offset, cum_bytes);

    passed = TRUE;

    /* If we're running a read filter, prime the epan_dissect_t with that
       filter. */
    if (n_rfilters > 0) {
        for(i = 0; i < n_rfcodes; i++) {
            epan_dissect_prime_dfilter(edt, rfcodes[i]);
        }
    }

    printf("%lu", (unsigned long int) cf->count);

    frame_data_set_before_dissect(&fdata, &cf->elapsed_time,
                                  &ref, prev_dis);

    if (ref == &fdata) {
       ref_frame = fdata;
       ref = &ref_frame;
    }

    /* We only need the columns if we're printing packet info but we're
     *not* verbose; in verbose mode, we print the protocol tree, not
     the protocol summary. */
    epan_dissect_run_with_taps(edt, cf->cd_t, whdr, frame_tvbuff_new(&fdata, pd), &fdata, &cf->cinfo);

    frame_data_set_after_dissect(&fdata, &cum_bytes);
    prev_dis_frame = fdata;
    prev_dis = &prev_dis_frame;

    prev_cap_frame = fdata;
    prev_cap = &prev_cap_frame;

    for(i = 0; i < n_rfilters; i++) {
        /* Run the read filter if we have one. */
        if (rfcodes[i])
            passed = dfilter_apply_edt(rfcodes[i], edt);
        else
            passed = TRUE;

        /* Print a one-line summary */
        printf(" %d", passed ? 1 : 0);
    }

    printf(" -\n");

    /* The ANSI C standard does not appear to *require* that a line-buffered
       stream be flushed to the host environment whenever a newline is
       written, it just says that, on such a stream, characters "are
       intended to be transmitted to or from the host environment as a
       block when a new-line character is encountered".

       The Visual C++ 6.0 C implementation doesn't do what is intended;
       even if you set a stream to be line-buffered, it still doesn't
       flush the buffer at the end of every line.

       So, if the "-l" flag was specified, we flush the standard output
       at the end of a packet.  This will do the right thing if we're
       printing packet summary lines, and, as we print the entire protocol
       tree for a single packet without waiting for anything to happen,
       it should be as good as line-buffered mode if we're printing
       protocol trees.  (The whole reason for the "-l" flag in either
       tcpdump or Rawshark is to allow the output of a live capture to
       be piped to a program or script and to have that script see the
       information for the packet as soon as it's printed, rather than
       having to wait until a standard I/O buffer fills up. */
    if (line_buffered)
        fflush(stdout);

    if (ferror(stdout)) {
        show_print_file_io_error(errno);
        exit(2);
    }

    epan_dissect_reset(edt);
    frame_data_destroy(&fdata);

    return passed;
}

/****************************************************************************************
 * FIELD EXTRACTION ROUTINES
 ****************************************************************************************/
typedef struct _pci_t {
    char *filter;
    int hf_index;
    int cmd_line_index;
} pci_t;

static const char* ftenum_to_string(header_field_info *hfi)
{
    const char* str;
    if (!hfi) {
        return "n.a.";
    }

    if (string_fmts->len > 0 && hfi->strings) {
        return "FT_STRING";
    }

    str = ftype_name(hfi->type);
    if (str == NULL) {
        str = "n.a.";
    }

    return str;
}

static void field_display_to_string(header_field_info *hfi, char* buf, int size)
{
    if (hfi->type != FT_BOOLEAN)
    {
        g_strlcpy(buf, proto_field_display_to_string(hfi->display), size);
    }
    else
    {
        g_snprintf(buf, size, "(Bit count: %d)", hfi->display);
    }
}

/*
 * Copied from various parts of proto.c
 */
#define FIELD_STR_INIT_LEN 256
#define cVALS(x) (const value_string*)(x)
static gboolean print_field_value(field_info *finfo, int cmd_line_index)
{
    header_field_info   *hfinfo;
    char                *fs_buf = NULL;
    char                *fs_ptr = NULL;
    static GString     *label_s = NULL;
    int                 fs_len;
    guint              i;
    string_fmt_t       *sf;
    guint32            uvalue;
    gint32             svalue;
    guint64            uvalue64;
    gint64             svalue64;
    const true_false_string *tfstring = &tfs_true_false;

    hfinfo = finfo->hfinfo;

    if (!label_s) {
        label_s = g_string_new("");
    }

    if(finfo->value.ftype->val_to_string_repr)
    {
        /*
         * this field has an associated value,
         * e.g: ip.hdr_len
         */
        fs_len = fvalue_string_repr_len(&finfo->value, FTREPR_DFILTER, finfo->hfinfo->display);
        fs_buf = fvalue_to_string_repr(NULL, &finfo->value,
                              FTREPR_DFILTER, finfo->hfinfo->display);
        fs_ptr = fs_buf;

        /* String types are quoted. Remove them. */
        if (IS_FT_STRING(finfo->value.ftype->ftype) && fs_len > 2) {
            fs_buf[fs_len - 1] = '\0';
            fs_ptr++;
        }
    }

    if (string_fmts->len > 0 && finfo->hfinfo->strings) {
        g_string_truncate(label_s, 0);
        for (i = 0; i < string_fmts->len; i++) {
            sf = (string_fmt_t *)g_ptr_array_index(string_fmts, i);
            if (sf->plain) {
                g_string_append(label_s, sf->plain);
            } else {
                switch (sf->format) {
                    case SF_NAME:
                        g_string_append(label_s, hfinfo->name);
                        break;
                    case SF_NUMVAL:
                        g_string_append(label_s, fs_ptr);
                        break;
                    case SF_STRVAL:
                        switch(hfinfo->type) {
                            case FT_BOOLEAN:
                                uvalue64 = fvalue_get_uinteger64(&finfo->value);
                                tfstring = (const struct true_false_string*) hfinfo->strings;
                                g_string_append(label_s, uvalue64 ? tfstring->true_string : tfstring->false_string);
                                break;
                            case FT_INT8:
                            case FT_INT16:
                            case FT_INT24:
                            case FT_INT32:
                                DISSECTOR_ASSERT(!hfinfo->bitmask);
                                svalue = fvalue_get_sinteger(&finfo->value);
                                if (hfinfo->display & BASE_RANGE_STRING) {
                                    g_string_append(label_s, rval_to_str_const(svalue, RVALS(hfinfo->strings), "Unknown"));
                                } else if (hfinfo->display & BASE_EXT_STRING) {
                                    g_string_append(label_s, val_to_str_ext_const(svalue, (value_string_ext *) hfinfo->strings, "Unknown"));
                                } else {
                                    g_string_append(label_s, val_to_str_const(svalue, cVALS(hfinfo->strings), "Unknown"));
                                }
                                break;
                            case FT_INT40: /* XXX: Shouldn't these be as smart as FT_INT{8,16,24,32}? */
                            case FT_INT48:
                            case FT_INT56:
                            case FT_INT64:
                                DISSECTOR_ASSERT(!hfinfo->bitmask);
                                svalue64 = (gint64)fvalue_get_sinteger64(&finfo->value);
                                if (hfinfo->display & BASE_VAL64_STRING) {
                                    g_string_append(label_s, val64_to_str_const(svalue64, (const val64_string *)(hfinfo->strings), "Unknown"));
                                }
                                break;
                            case FT_UINT8:
                            case FT_UINT16:
                            case FT_UINT24:
                            case FT_UINT32:
                                DISSECTOR_ASSERT(!hfinfo->bitmask);
                                uvalue = fvalue_get_uinteger(&finfo->value);
                                if (!hfinfo->bitmask && hfinfo->display & BASE_RANGE_STRING) {
                                    g_string_append(label_s, rval_to_str_const(uvalue, RVALS(hfinfo->strings), "Unknown"));
                                } else if (hfinfo->display & BASE_EXT_STRING) {
                                    g_string_append(label_s, val_to_str_ext_const(uvalue, (value_string_ext *) hfinfo->strings, "Unknown"));
                                } else {
                                    g_string_append(label_s, val_to_str_const(uvalue, cVALS(hfinfo->strings), "Unknown"));
                                }
                                break;
                            case FT_UINT40: /* XXX: Shouldn't these be as smart as FT_INT{8,16,24,32}? */
                            case FT_UINT48:
                            case FT_UINT56:
                            case FT_UINT64:
                                DISSECTOR_ASSERT(!hfinfo->bitmask);
                                uvalue64 = fvalue_get_uinteger64(&finfo->value);
                                if (hfinfo->display & BASE_VAL64_STRING) {
                                    g_string_append(label_s, val64_to_str_const(uvalue64, (const val64_string *)(hfinfo->strings), "Unknown"));
                                }
                                break;
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        printf(" %d=\"%s\"", cmd_line_index, label_s->str);
        wmem_free(NULL, fs_buf);
        return TRUE;
    }

    if(finfo->value.ftype->val_to_string_repr)
    {
        printf(" %d=\"%s\"", cmd_line_index, fs_ptr);
        wmem_free(NULL, fs_buf);
        return TRUE;
    }

    /*
     * This field doesn't have an associated value,
     * e.g. http
     * We return n.a.
     */
    printf(" %d=\"n.a.\"", cmd_line_index);
    return TRUE;
}

static int
protocolinfo_packet(void *prs, packet_info *pinfo _U_, epan_dissect_t *edt, const void *dummy _U_)
{
    pci_t *rs=(pci_t *)prs;
    GPtrArray *gp;
    guint i;

    gp=proto_get_finfo_ptr_array(edt->tree, rs->hf_index);
    if(!gp){
        printf(" n.a.");
        return 0;
    }

    /*
     * Print each occurrence of the field
     */
    for (i = 0; i < gp->len; i++) {
        print_field_value((field_info *)gp->pdata[i], rs->cmd_line_index);
    }

    return 0;
}

int g_cmd_line_index = 0;

/*
 * field must be persistent - we don't g_strdup() it below
 */
static void
protocolinfo_init(char *field)
{
    pci_t *rs;
    header_field_info *hfi;
    GString *error_string;
    char hfibuf[100];

    hfi=proto_registrar_get_byname(field);
    if(!hfi){
        fprintf(stderr, "rawshark: Field \"%s\" doesn't exist.\n", field);
        exit(1);
    }

    field_display_to_string(hfi, hfibuf, sizeof(hfibuf));
    printf("%d %s %s - ",
            g_cmd_line_index,
            ftenum_to_string(hfi),
            hfibuf);

    rs=(pci_t *)g_malloc(sizeof(pci_t));
    rs->hf_index=hfi->id;
    rs->filter=field;
    rs->cmd_line_index = g_cmd_line_index++;

    error_string=register_tap_listener("frame", rs, rs->filter, TL_REQUIRES_PROTO_TREE, NULL, protocolinfo_packet, NULL);
    if(error_string){
        /* error, we failed to attach to the tap. complain and clean up */
        fprintf(stderr, "rawshark: Couldn't register field extraction tap: %s\n",
                error_string->str);
        g_string_free(error_string, TRUE);
        if(rs->filter){
            g_free(rs->filter);
        }
        g_free(rs);

        exit(1);
    }
}

/*
 * Given a format string, split it into a GPtrArray of string_fmt_t structs
 * and fill in string_fmt_parts.
 */

static void
add_string_fmt(string_fmt_e format, gchar *plain) {
    string_fmt_t *sf = (string_fmt_t *)g_malloc(sizeof(string_fmt_t));

    sf->format = format;
    sf->plain = g_strdup(plain);

    g_ptr_array_add(string_fmts, sf);
}

static gboolean
parse_field_string_format(gchar *format) {
    GString *plain_s = g_string_new("");
    size_t len;
    size_t pos = 0;

    if (!format) {
        return FALSE;
    }

    len = strlen(format);
    g_ptr_array_set_size(string_fmts, 0);

    while (pos < len) {
        if (format[pos] == '%') {
            if (pos >= len) { /* There should always be a following character */
                return FALSE;
            }
            pos++;
            if (plain_s->len > 0) {
                add_string_fmt(SF_NONE, plain_s->str);
                g_string_truncate(plain_s, 0);
            }
            switch (format[pos]) {
                case 'D':
                    add_string_fmt(SF_NAME, NULL);
                    break;
                case 'N':
                    add_string_fmt(SF_NUMVAL, NULL);
                    break;
                case 'S':
                    add_string_fmt(SF_STRVAL, NULL);
                    break;
                case '%':
                    g_string_append_c(plain_s, '%');
                    break;
                default: /* Invalid format */
                    return FALSE;
            }
        } else {
            g_string_append_c(plain_s, format[pos]);
        }
        pos++;
    }

    if (plain_s->len > 0) {
        add_string_fmt(SF_NONE, plain_s->str);
    }
    g_string_free(plain_s, TRUE);

    return TRUE;
}
/****************************************************************************************
 * END OF FIELD EXTRACTION ROUTINES
 ****************************************************************************************/

static void
show_print_file_io_error(int err)
{
    switch (err) {

        case ENOSPC:
            cmdarg_err("Not all the packets could be printed because there is "
                       "no space left on the file system.");
            break;

#ifdef EDQUOT
        case EDQUOT:
            cmdarg_err("Not all the packets could be printed because you are "
                       "too close to, or over your disk quota.");
            break;
#endif

        default:
            cmdarg_err("An error occurred while printing packets: %s.",
                       g_strerror(err));
            break;
    }
}

/*
 * Open/create errors are reported with an console message in Rawshark.
 */
static void
open_failure_message(const char *filename, int err, gboolean for_writing)
{
    fprintf(stderr, "rawshark: ");
    fprintf(stderr, file_open_error_message(err, for_writing), filename);
    fprintf(stderr, "\n");
}

static const nstime_t *
raw_get_frame_ts(void *data _U_, guint32 frame_num)
{
    if (ref && ref->num == frame_num)
        return &ref->abs_ts;

    if (prev_dis && prev_dis->num == frame_num)
        return &prev_dis->abs_ts;

    if (prev_cap && prev_cap->num == frame_num)
        return &prev_cap->abs_ts;

    return NULL;
}

static epan_t *
raw_epan_new(capture_file *cf)
{
    epan_t *epan = epan_new();

    epan->data = cf;
    epan->get_frame_ts = raw_get_frame_ts;
    epan->get_interface_name = cap_file_get_interface_name;
    epan->get_user_comment = NULL;

    return epan;
}

cf_status_t
raw_cf_open(capture_file *cf, const char *fname)
{
    if ((fd = raw_pipe_open(fname)) < 0)
        return CF_ERROR;

    /* The open succeeded.  Fill in the information for this file. */

    /* Create new epan session for dissection. */
    epan_free(cf->epan);
    cf->epan = raw_epan_new(cf);

    cf->wth = NULL;
    cf->f_datalen = 0; /* not used, but set it anyway */

    /* Set the file name because we need it to set the follow stream filter.
       XXX - is that still true?  We need it for other reasons, though,
       in any case. */
    cf->filename = g_strdup(fname);

    /* Indicate whether it's a permanent or temporary file. */
    cf->is_tempfile = FALSE;

    /* No user changes yet. */
    cf->unsaved_changes = FALSE;

    cf->cd_t      = WTAP_FILE_TYPE_SUBTYPE_UNKNOWN;
    cf->open_type = WTAP_TYPE_AUTO;
    cf->count     = 0;
    cf->drops_known = FALSE;
    cf->drops     = 0;
    cf->has_snap = FALSE;
    cf->snap = WTAP_MAX_PACKET_SIZE;
    nstime_set_zero(&cf->elapsed_time);
    ref = NULL;
    prev_dis = NULL;
    prev_cap = NULL;

    return CF_OK;
}


/*
 * General errors are reported with an console message in Rawshark.
 */
static void
failure_message(const char *msg_format, va_list ap)
{
    fprintf(stderr, "rawshark: ");
    vfprintf(stderr, msg_format, ap);
    fprintf(stderr, "\n");
}

/*
 * Read errors are reported with an console message in Rawshark.
 */
static void
read_failure_message(const char *filename, int err)
{
    cmdarg_err("An error occurred while reading from the file \"%s\": %s.",
               filename, g_strerror(err));
}

/*
 * Write errors are reported with an console message in Rawshark.
 */
static void
write_failure_message(const char *filename, int err)
{
    cmdarg_err("An error occurred while writing to the file \"%s\": %s.",
               filename, g_strerror(err));
}

/*
 * Report an error in command-line arguments.
 */
static void
rawshark_cmdarg_err(const char *fmt, va_list ap)
{
    fprintf(stderr, "rawshark: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

/*
 * Report additional information for an error in command-line arguments.
 */
static void
rawshark_cmdarg_err_cont(const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
