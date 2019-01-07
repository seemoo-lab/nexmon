/* capture_opts.c
 * Routines for capture options setting
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

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_LIBPCAP

#include <string.h>

#include <errno.h>

#include <glib.h>

#include "capture_opts.h"
#include "ringbuffer.h"

#include <wsutil/clopts_common.h>
#include <wsutil/cmdarg_err.h>
#include <wsutil/file_util.h>

#include "caputils/capture_ifinfo.h"
#include "caputils/capture-pcap-util.h"

#include "filter_files.h"

static gboolean capture_opts_output_to_pipe(const char *save_file, gboolean *is_pipe);


void
capture_opts_init(capture_options *capture_opts)
{
    capture_opts->ifaces                          = g_array_new(FALSE, FALSE, sizeof(interface_options));
    capture_opts->all_ifaces                      = g_array_new(FALSE, FALSE, sizeof(interface_t));
    capture_opts->num_selected                    = 0;
    capture_opts->default_options.name            = NULL;
    capture_opts->default_options.descr           = NULL;
    capture_opts->default_options.cfilter         = NULL;
    capture_opts->default_options.has_snaplen     = FALSE;
    capture_opts->default_options.snaplen         = WTAP_MAX_PACKET_SIZE;
    capture_opts->default_options.linktype        = -1; /* use interface default */
    capture_opts->default_options.promisc_mode    = TRUE;
    capture_opts->default_options.if_type         = IF_WIRED;
#ifdef HAVE_EXTCAP
    capture_opts->default_options.extcap          = NULL;
    capture_opts->default_options.extcap_fifo     = NULL;
    capture_opts->default_options.extcap_args     = NULL;
    capture_opts->default_options.extcap_userdata = NULL;
    capture_opts->default_options.extcap_pid      = INVALID_EXTCAP_PID;
#endif
#ifdef CAN_SET_CAPTURE_BUFFER_SIZE
    capture_opts->default_options.buffer_size     = DEFAULT_CAPTURE_BUFFER_SIZE;
#endif
    capture_opts->default_options.monitor_mode    = FALSE;
#ifdef HAVE_PCAP_REMOTE
    capture_opts->default_options.src_type        = CAPTURE_IFLOCAL;
    capture_opts->default_options.remote_host     = NULL;
    capture_opts->default_options.remote_port     = NULL;
    capture_opts->default_options.auth_type       = CAPTURE_AUTH_NULL;
    capture_opts->default_options.auth_username   = NULL;
    capture_opts->default_options.auth_password   = NULL;
    capture_opts->default_options.datatx_udp      = FALSE;
    capture_opts->default_options.nocap_rpcap     = TRUE;
    capture_opts->default_options.nocap_local     = FALSE;
#endif
#ifdef HAVE_PCAP_SETSAMPLING
    capture_opts->default_options.sampling_method = CAPTURE_SAMP_NONE;
    capture_opts->default_options.sampling_param  = 0;
#endif
    capture_opts->saving_to_file                  = FALSE;
    capture_opts->save_file                       = NULL;
    capture_opts->group_read_access               = FALSE;
#ifdef PCAP_NG_DEFAULT
    capture_opts->use_pcapng                      = TRUE;             /* Save as pcap-ng by default */
#else
    capture_opts->use_pcapng                      = FALSE;            /* Save as pcap by default */
#endif
    capture_opts->real_time_mode                  = TRUE;
    capture_opts->show_info                       = TRUE;
    capture_opts->restart                         = FALSE;
    capture_opts->orig_save_file                  = NULL;

    capture_opts->multi_files_on                  = FALSE;
    capture_opts->has_file_duration               = FALSE;
    capture_opts->file_duration                   = 60;               /* 1 min */
    capture_opts->has_ring_num_files              = FALSE;
    capture_opts->ring_num_files                  = RINGBUFFER_MIN_NUM_FILES;

    capture_opts->has_autostop_files              = FALSE;
    capture_opts->autostop_files                  = 1;
    capture_opts->has_autostop_packets            = FALSE;
    capture_opts->autostop_packets                = 0;
    capture_opts->has_autostop_filesize           = FALSE;
    capture_opts->autostop_filesize               = 1000;             /* 1 MB */
    capture_opts->has_autostop_duration           = FALSE;
    capture_opts->autostop_duration               = 60;               /* 1 min */
    capture_opts->capture_comment                 = NULL;

    capture_opts->output_to_pipe                  = FALSE;
    capture_opts->capture_child                   = FALSE;
}


/* log content of capture_opts */
void
capture_opts_log(const char *log_domain, GLogLevelFlags log_level, capture_options *capture_opts) {
    guint i;

    g_log(log_domain, log_level, "CAPTURE OPTIONS     :");

    for (i = 0; i < capture_opts->ifaces->len; i++) {
        interface_options interface_opts;

        interface_opts = g_array_index(capture_opts->ifaces, interface_options, i);
        g_log(log_domain, log_level, "Interface name[%02d]  : %s", i, interface_opts.name ? interface_opts.name : "(unspecified)");
        g_log(log_domain, log_level, "Interface description[%02d] : %s", i, interface_opts.descr ? interface_opts.descr : "(unspecified)");
        g_log(log_domain, log_level, "Console display name[%02d]: %s", i, interface_opts.console_display_name ? interface_opts.console_display_name : "(unspecified)");
        g_log(log_domain, log_level, "Capture filter[%02d]  : %s", i, interface_opts.cfilter ? interface_opts.cfilter : "(unspecified)");
        g_log(log_domain, log_level, "Snap length[%02d] (%u) : %d", i, interface_opts.has_snaplen, interface_opts.snaplen);
        g_log(log_domain, log_level, "Link Type[%02d]       : %d", i, interface_opts.linktype);
        g_log(log_domain, log_level, "Promiscuous Mode[%02d]: %s", i, interface_opts.promisc_mode?"TRUE":"FALSE");
#ifdef HAVE_EXTCAP
        g_log(log_domain, log_level, "Extcap[%02d]          : %s", i, interface_opts.extcap ? interface_opts.extcap : "(unspecified)");
        g_log(log_domain, log_level, "Extcap FIFO[%02d]     : %s", i, interface_opts.extcap_fifo ? interface_opts.extcap_fifo : "(unspecified)");
        g_log(log_domain, log_level, "Extcap PID[%02d]      : %d", i, interface_opts.extcap_pid);
#endif
#ifdef CAN_SET_CAPTURE_BUFFER_SIZE
        g_log(log_domain, log_level, "Buffer size[%02d]     : %d (MB)", i, interface_opts.buffer_size);
#endif
        g_log(log_domain, log_level, "Monitor Mode[%02d]    : %s", i, interface_opts.monitor_mode?"TRUE":"FALSE");
#ifdef HAVE_PCAP_REMOTE
        g_log(log_domain, log_level, "Capture source[%02d]  : %s", i,
            interface_opts.src_type == CAPTURE_IFLOCAL ? "Local interface" :
            interface_opts.src_type == CAPTURE_IFREMOTE ? "Remote interface" :
            "Unknown");
        if (interface_opts.src_type == CAPTURE_IFREMOTE) {
            g_log(log_domain, log_level, "Remote host[%02d]     : %s", i, interface_opts.remote_host ? interface_opts.remote_host : "(unspecified)");
            g_log(log_domain, log_level, "Remote port[%02d]     : %s", i, interface_opts.remote_port ? interface_opts.remote_port : "(unspecified)");
        }
        g_log(log_domain, log_level, "Authentication[%02d]  : %s", i,
            interface_opts.auth_type == CAPTURE_AUTH_NULL ? "Null" :
            interface_opts.auth_type == CAPTURE_AUTH_PWD ? "By username/password" :
            "Unknown");
        if (interface_opts.auth_type == CAPTURE_AUTH_PWD) {
            g_log(log_domain, log_level, "Auth username[%02d]   : %s", i, interface_opts.auth_username ? interface_opts.auth_username : "(unspecified)");
            g_log(log_domain, log_level, "Auth password[%02d]   : <hidden>", i);
        }
        g_log(log_domain, log_level, "UDP data tfer[%02d]   : %u", i, interface_opts.datatx_udp);
        g_log(log_domain, log_level, "No cap. RPCAP[%02d]   : %u", i, interface_opts.nocap_rpcap);
        g_log(log_domain, log_level, "No cap. local[%02d]   : %u", i, interface_opts.nocap_local);
#endif
#ifdef HAVE_PCAP_SETSAMPLING
        g_log(log_domain, log_level, "Sampling meth.[%02d]  : %d", i, interface_opts.sampling_method);
        g_log(log_domain, log_level, "Sampling param.[%02d] : %d", i, interface_opts.sampling_param);
#endif
    }
    g_log(log_domain, log_level, "Interface name[df]  : %s", capture_opts->default_options.name ? capture_opts->default_options.name : "(unspecified)");
    g_log(log_domain, log_level, "Interface Descr[df] : %s", capture_opts->default_options.descr ? capture_opts->default_options.descr : "(unspecified)");
    g_log(log_domain, log_level, "Capture filter[df]  : %s", capture_opts->default_options.cfilter ? capture_opts->default_options.cfilter : "(unspecified)");
    g_log(log_domain, log_level, "Snap length[df] (%u) : %d", capture_opts->default_options.has_snaplen, capture_opts->default_options.snaplen);
    g_log(log_domain, log_level, "Link Type[df]       : %d", capture_opts->default_options.linktype);
    g_log(log_domain, log_level, "Promiscuous Mode[df]: %s", capture_opts->default_options.promisc_mode?"TRUE":"FALSE");
#ifdef HAVE_EXTCAP
    g_log(log_domain, log_level, "Extcap[df]          : %s", capture_opts->default_options.extcap ? capture_opts->default_options.extcap : "(unspecified)");
    g_log(log_domain, log_level, "Extcap FIFO[df]     : %s", capture_opts->default_options.extcap_fifo ? capture_opts->default_options.extcap_fifo : "(unspecified)");
#endif
#ifdef CAN_SET_CAPTURE_BUFFER_SIZE
    g_log(log_domain, log_level, "Buffer size[df]     : %d (MB)", capture_opts->default_options.buffer_size);
#endif
    g_log(log_domain, log_level, "Monitor Mode[df]    : %s", capture_opts->default_options.monitor_mode?"TRUE":"FALSE");
#ifdef HAVE_PCAP_REMOTE
    g_log(log_domain, log_level, "Capture source[df]  : %s",
        capture_opts->default_options.src_type == CAPTURE_IFLOCAL ? "Local interface" :
        capture_opts->default_options.src_type == CAPTURE_IFREMOTE ? "Remote interface" :
        "Unknown");
    if (capture_opts->default_options.src_type == CAPTURE_IFREMOTE) {
        g_log(log_domain, log_level, "Remote host[df]     : %s", capture_opts->default_options.remote_host ? capture_opts->default_options.remote_host : "(unspecified)");
        g_log(log_domain, log_level, "Remote port[df]     : %s", capture_opts->default_options.remote_port ? capture_opts->default_options.remote_port : "(unspecified)");
    }
    g_log(log_domain, log_level, "Authentication[df]  : %s",
        capture_opts->default_options.auth_type == CAPTURE_AUTH_NULL ? "Null" :
        capture_opts->default_options.auth_type == CAPTURE_AUTH_PWD ? "By username/password" :
        "Unknown");
    if (capture_opts->default_options.auth_type == CAPTURE_AUTH_PWD) {
        g_log(log_domain, log_level, "Auth username[df]   : %s", capture_opts->default_options.auth_username ? capture_opts->default_options.auth_username : "(unspecified)");
        g_log(log_domain, log_level, "Auth password[df]   : <hidden>");
    }
    g_log(log_domain, log_level, "UDP data tfer[df]   : %u", capture_opts->default_options.datatx_udp);
    g_log(log_domain, log_level, "No cap. RPCAP[df]   : %u", capture_opts->default_options.nocap_rpcap);
    g_log(log_domain, log_level, "No cap. local[df]   : %u", capture_opts->default_options.nocap_local);
#endif
#ifdef HAVE_PCAP_SETSAMPLING
    g_log(log_domain, log_level, "Sampling meth. [df] : %d", capture_opts->default_options.sampling_method);
    g_log(log_domain, log_level, "Sampling param.[df] : %d", capture_opts->default_options.sampling_param);
#endif
    g_log(log_domain, log_level, "SavingToFile        : %u", capture_opts->saving_to_file);
    g_log(log_domain, log_level, "SaveFile            : %s", (capture_opts->save_file) ? capture_opts->save_file : "");
    g_log(log_domain, log_level, "GroupReadAccess     : %u", capture_opts->group_read_access);
    g_log(log_domain, log_level, "Fileformat          : %s", (capture_opts->use_pcapng) ? "PCAPNG" : "PCAP");
    g_log(log_domain, log_level, "RealTimeMode        : %u", capture_opts->real_time_mode);
    g_log(log_domain, log_level, "ShowInfo            : %u", capture_opts->show_info);

    g_log(log_domain, log_level, "MultiFilesOn        : %u", capture_opts->multi_files_on);
    g_log(log_domain, log_level, "FileDuration    (%u) : %u", capture_opts->has_file_duration, capture_opts->file_duration);
    g_log(log_domain, log_level, "RingNumFiles    (%u) : %u", capture_opts->has_ring_num_files, capture_opts->ring_num_files);

    g_log(log_domain, log_level, "AutostopFiles   (%u) : %u", capture_opts->has_autostop_files, capture_opts->autostop_files);
    g_log(log_domain, log_level, "AutostopPackets (%u) : %u", capture_opts->has_autostop_packets, capture_opts->autostop_packets);
    g_log(log_domain, log_level, "AutostopFilesize(%u) : %u (KB)", capture_opts->has_autostop_filesize, capture_opts->autostop_filesize);
    g_log(log_domain, log_level, "AutostopDuration(%u) : %u", capture_opts->has_autostop_duration, capture_opts->autostop_duration);
}

/*
 * Given a string of the form "<autostop criterion>:<value>", as might appear
 * as an argument to a "-a" option, parse it and set the criterion in
 * question.  Return an indication of whether it succeeded or failed
 * in some fashion.
 */
static gboolean
set_autostop_criterion(capture_options *capture_opts, const char *autostoparg)
{
    gchar *p, *colonp;

    colonp = strchr(autostoparg, ':');
    if (colonp == NULL)
        return FALSE;

    p = colonp;
    *p++ = '\0';

    /*
     * Skip over any white space (there probably won't be any, but
     * as we allow it in the preferences file, we might as well
     * allow it here).
     */
    while (g_ascii_isspace(*p))
        p++;
    if (*p == '\0') {
        /*
         * Put the colon back, so if our caller uses, in an
         * error message, the string they passed us, the message
         * looks correct.
         */
        *colonp = ':';
        return FALSE;
    }
    if (strcmp(autostoparg,"duration") == 0) {
        capture_opts->has_autostop_duration = TRUE;
        capture_opts->autostop_duration = get_positive_int(p,"autostop duration");
    } else if (strcmp(autostoparg,"filesize") == 0) {
        capture_opts->has_autostop_filesize = TRUE;
        capture_opts->autostop_filesize = get_positive_int(p,"autostop filesize");
    } else if (strcmp(autostoparg,"files") == 0) {
        capture_opts->multi_files_on = TRUE;
        capture_opts->has_autostop_files = TRUE;
        capture_opts->autostop_files = get_positive_int(p,"autostop files");
    } else {
        return FALSE;
    }
    *colonp = ':'; /* put the colon back */
    return TRUE;
}

static gboolean get_filter_arguments(capture_options* capture_opts, const char* arg)
{
    char* colonp;
    char* val;
    char* filter_exp = NULL;

    colonp = strchr(arg, ':');
    if (colonp) {
        val = colonp;
        *val = '\0';
        val++;
        if (strcmp(arg, "predef") == 0) {
            GList* filterItem;

            filterItem = get_filter_list_first(CFILTER_LIST);
            while (filterItem != NULL) {
                filter_def *filterDef;

                filterDef = (filter_def*)filterItem->data;
                if (strcmp(val, filterDef->name) == 0) {
                    filter_exp = g_strdup(filterDef->strval);
                    break;
                }
                filterItem = filterItem->next;
            }
        }
    }

    if (filter_exp == NULL) {
        /* No filter expression found yet; fallback to previous implemention
           and assume the arg contains a filter expression */
        if (colonp) {
            *colonp = ':';      /* restore colon */
        }
        filter_exp = g_strdup(arg);
    }

    if (capture_opts->ifaces->len > 0) {
        interface_options interface_opts;

        interface_opts = g_array_index(capture_opts->ifaces, interface_options, capture_opts->ifaces->len - 1);
        capture_opts->ifaces = g_array_remove_index(capture_opts->ifaces, capture_opts->ifaces->len - 1);
        g_free(interface_opts.cfilter);
        interface_opts.cfilter = filter_exp;
        g_array_append_val(capture_opts->ifaces, interface_opts);
        return TRUE;
    }
    else {
        g_free(capture_opts->default_options.cfilter);
        capture_opts->default_options.cfilter = filter_exp;
        return TRUE;
    }
}

/*
 * Given a string of the form "<ring buffer file>:<duration>", as might appear
 * as an argument to a "-b" option, parse it and set the arguments in
 * question.  Return an indication of whether it succeeded or failed
 * in some fashion.
 */
static gboolean
get_ring_arguments(capture_options *capture_opts, const char *arg)
{
    gchar *p = NULL, *colonp;

    colonp = strchr(arg, ':');
    if (colonp == NULL)
        return FALSE;

    p = colonp;
    *p++ = '\0';

    /*
     * Skip over any white space (there probably won't be any, but
     * as we allow it in the preferences file, we might as well
     * allow it here).
     */
    while (g_ascii_isspace(*p))
        p++;
    if (*p == '\0') {
        /*
         * Put the colon back, so if our caller uses, in an
         * error message, the string they passed us, the message
         * looks correct.
         */
        *colonp = ':';
        return FALSE;
    }

    if (strcmp(arg,"files") == 0) {
        capture_opts->has_ring_num_files = TRUE;
        capture_opts->ring_num_files = get_positive_int(p, "number of ring buffer files");
    } else if (strcmp(arg,"filesize") == 0) {
        capture_opts->has_autostop_filesize = TRUE;
        capture_opts->autostop_filesize = get_positive_int(p, "ring buffer filesize");
    } else if (strcmp(arg,"duration") == 0) {
        capture_opts->has_file_duration = TRUE;
        capture_opts->file_duration = get_positive_int(p, "ring buffer duration");
    }

    *colonp = ':';    /* put the colon back */
    return TRUE;
}

#ifdef HAVE_PCAP_SETSAMPLING
/*
 * Given a string of the form "<sampling type>:<value>", as might appear
 * as an argument to a "-m" option, parse it and set the arguments in
 * question.  Return an indication of whether it succeeded or failed
 * in some fashion.
 */
static gboolean
get_sampling_arguments(capture_options *capture_opts, const char *arg)
{
    gchar *p = NULL, *colonp;

    colonp = strchr(arg, ':');
    if (colonp == NULL)
        return FALSE;

    p = colonp;
    *p++ = '\0';

    while (g_ascii_isspace(*p))
        p++;
    if (*p == '\0') {
        *colonp = ':';
        return FALSE;
    }

    if (strcmp(arg, "count") == 0) {
        if (capture_opts->ifaces->len > 0) {
            interface_options interface_opts;

            interface_opts = g_array_index(capture_opts->ifaces, interface_options, capture_opts->ifaces->len - 1);
            capture_opts->ifaces = g_array_remove_index(capture_opts->ifaces, capture_opts->ifaces->len - 1);
            interface_opts.sampling_method = CAPTURE_SAMP_BY_COUNT;
            interface_opts.sampling_param = get_positive_int(p, "sampling count");
            g_array_append_val(capture_opts->ifaces, interface_opts);
        } else {
            capture_opts->default_options.sampling_method = CAPTURE_SAMP_BY_COUNT;
            capture_opts->default_options.sampling_param = get_positive_int(p, "sampling count");
        }
    } else if (strcmp(arg, "timer") == 0) {
        if (capture_opts->ifaces->len > 0) {
            interface_options interface_opts;

            interface_opts = g_array_index(capture_opts->ifaces, interface_options, capture_opts->ifaces->len - 1);
            capture_opts->ifaces = g_array_remove_index(capture_opts->ifaces, capture_opts->ifaces->len - 1);
            interface_opts.sampling_method = CAPTURE_SAMP_BY_TIMER;
            interface_opts.sampling_param = get_positive_int(p, "sampling timer");
            g_array_append_val(capture_opts->ifaces, interface_opts);
        } else {
            capture_opts->default_options.sampling_method = CAPTURE_SAMP_BY_TIMER;
            capture_opts->default_options.sampling_param = get_positive_int(p, "sampling timer");
        }
    }
    *colonp = ':';
    return TRUE;
}
#endif

#ifdef HAVE_PCAP_REMOTE
/*
 * Given a string of the form "<username>:<password>", as might appear
 * as an argument to a "-A" option, parse it and set the arguments in
 * question.  Return an indication of whether it succeeded or failed
 * in some fashion.
 */
static gboolean
get_auth_arguments(capture_options *capture_opts, const char *arg)
{
    gchar *p = NULL, *colonp;

    colonp = strchr(arg, ':');
    if (colonp == NULL)
        return FALSE;

    p = colonp;
    *p++ = '\0';

    while (g_ascii_isspace(*p))
        p++;

    if (capture_opts->ifaces->len > 0) {
        interface_options interface_opts;

        interface_opts = g_array_index(capture_opts->ifaces, interface_options, capture_opts->ifaces->len - 1);
        capture_opts->ifaces = g_array_remove_index(capture_opts->ifaces, capture_opts->ifaces->len - 1);
        interface_opts.auth_type = CAPTURE_AUTH_PWD;
        interface_opts.auth_username = g_strdup(arg);
        interface_opts.auth_password = g_strdup(p);
        g_array_append_val(capture_opts->ifaces, interface_opts);
    } else {
        capture_opts->default_options.auth_type = CAPTURE_AUTH_PWD;
        capture_opts->default_options.auth_username = g_strdup(arg);
        capture_opts->default_options.auth_password = g_strdup(p);
    }
    *colonp = ':';
    return TRUE;
}
#endif

static int
capture_opts_add_iface_opt(capture_options *capture_opts, const char *optarg_str_p)
{
    long        adapter_index;
    char        *p;
    GList       *if_list;
    if_info_t   *if_info;
    int         err;
    gchar       *err_str;
    interface_options interface_opts;

    /*
     * If the argument is a number, treat it as an index into the list
     * of adapters, as printed by "tshark -D".
     *
     * This should be OK on UNIX systems, as interfaces shouldn't have
     * names that begin with digits.  It can be useful on Windows, where
     * more than one interface can have the same name.
     */
    adapter_index = strtol(optarg_str_p, &p, 10);
    if (p != NULL && *p == '\0') {
        if (adapter_index < 0) {
            cmdarg_err("The specified adapter index is a negative number");
            return 1;
        }
        if (adapter_index > INT_MAX) {
            cmdarg_err("The specified adapter index is too large (greater than %d)",
                       INT_MAX);
            return 1;
        }
        if (adapter_index == 0) {
            cmdarg_err("There is no interface with that adapter index");
            return 1;
        }
        if_list = capture_interface_list(&err, &err_str, NULL);
        if (if_list == NULL) {
            if (err == 0)
                cmdarg_err("There are no interfaces on which a capture can be done");
            else {
                cmdarg_err("%s", err_str);
                g_free(err_str);
            }
            return 2;
        }
        if_info = (if_info_t *)g_list_nth_data(if_list, (int)(adapter_index - 1));
        if (if_info == NULL) {
            cmdarg_err("There is no interface with that adapter index");
            return 1;
        }
        interface_opts.name = g_strdup(if_info->name);
        if (if_info->friendly_name != NULL) {
            /*
             * We have a friendly name for the interface, so display that
             * instead of the interface name/guid.
             *
             * XXX - on UN*X, the interface name is not quite so ugly,
             * and might be more familiar to users; display them both?
             */
            interface_opts.console_display_name = g_strdup(if_info->friendly_name);
        } else {
            /* fallback to the interface name */
            interface_opts.console_display_name = g_strdup(if_info->name);
        }
        interface_opts.if_type = if_info->type;
#ifdef HAVE_EXTCAP
        interface_opts.extcap = g_strdup(if_info->extcap);
#endif
        free_interface_list(if_list);
    } else if (capture_opts->capture_child) {
        /* In Wireshark capture child mode, thus proper device name is supplied. */
        /* No need for trying to match it for friendly names. */
        interface_opts.name = g_strdup(optarg_str_p);
        interface_opts.console_display_name = g_strdup(optarg_str_p);
        interface_opts.if_type = capture_opts->default_options.if_type;
#ifdef HAVE_EXTCAP
        interface_opts.extcap = g_strdup(capture_opts->default_options.extcap);
#endif
    } else {
        /*
         * Retrieve the interface list so that we can search for the
         * specified option amongst both the interface names and the
         * friendly names and so that we find the friendly name even
         * if an interface name was specified.
         *
         * If we can't get the list, just use the specified option as
         * the interface name, so that the user can try specifying an
         * interface explicitly for testing purposes.
         */
        if_list = capture_interface_list(&err, NULL, NULL);
        if (if_list != NULL) {
            /* try and do an exact match (case insensitive) */
            GList   *if_entry;
            gboolean matched;

            matched = FALSE;
            for (if_entry = g_list_first(if_list); if_entry != NULL;
                 if_entry = g_list_next(if_entry))
            {
                if_info = (if_info_t *)if_entry->data;
                /* exact name check */
                if (g_ascii_strcasecmp(if_info->name, optarg_str_p) == 0) {
                    /* exact match on the interface name, use that for displaying etc */
                    interface_opts.name = g_strdup(if_info->name);

                    if (if_info->friendly_name != NULL) {
                        /*
                         * If we have a friendly name, use that for the
                         * console display name, as it is the basis for
                         * the auto generated temp filename.
                         */
                        interface_opts.console_display_name = g_strdup(if_info->friendly_name);
                    } else {
                        interface_opts.console_display_name = g_strdup(if_info->name);
                    }
                    interface_opts.if_type = if_info->type;
#ifdef HAVE_EXTCAP
                    interface_opts.extcap = g_strdup(if_info->extcap);
#endif
                    matched = TRUE;
                    break;
                }

                /* exact friendly name check */
                if (if_info->friendly_name != NULL &&
                    g_ascii_strcasecmp(if_info->friendly_name, optarg_str_p) == 0) {
                    /* exact match - use the friendly name for display */
                    interface_opts.name = g_strdup(if_info->name);
                    interface_opts.console_display_name = g_strdup(if_info->friendly_name);
                    interface_opts.if_type = if_info->type;
#ifdef HAVE_EXTCAP
                    interface_opts.extcap = g_strdup(if_info->extcap);
#endif
                    matched = TRUE;
                    break;
                }
            }

            /* didn't find, attempt a case insensitive prefix match of the friendly name*/
            if (!matched) {
                size_t prefix_length;

                prefix_length = strlen(optarg_str_p);
                for (if_entry = g_list_first(if_list); if_entry != NULL;
                     if_entry = g_list_next(if_entry))
                {
                    if_info = (if_info_t *)if_entry->data;

                    if (if_info->friendly_name != NULL &&
                        g_ascii_strncasecmp(if_info->friendly_name, optarg_str_p, prefix_length) == 0) {
                        /* prefix match - use the friendly name for display */
                        interface_opts.name = g_strdup(if_info->name);
                        interface_opts.console_display_name = g_strdup(if_info->friendly_name);
                        interface_opts.if_type = if_info->type;
#ifdef HAVE_EXTCAP
                        interface_opts.extcap = g_strdup(if_info->extcap);
#endif
                        matched = TRUE;
                        break;
                    }
                }
            }
            if (!matched) {
                /*
                 * We didn't find the interface in the list; just use
                 * the specified name, so that, for example, if an
                 * interface doesn't show up in the list for some
                 * reason, the user can try specifying it explicitly
                 * for testing purposes.
                 */
                interface_opts.name = g_strdup(optarg_str_p);
                interface_opts.console_display_name = g_strdup(optarg_str_p);
                interface_opts.if_type = capture_opts->default_options.if_type;
#ifdef HAVE_EXTCAP
                interface_opts.extcap = g_strdup(capture_opts->default_options.extcap);
#endif
            }
            free_interface_list(if_list);
        } else {
            interface_opts.name = g_strdup(optarg_str_p);
            interface_opts.console_display_name = g_strdup(optarg_str_p);
            interface_opts.if_type = capture_opts->default_options.if_type;
#ifdef HAVE_EXTCAP
            interface_opts.extcap = g_strdup(capture_opts->default_options.extcap);
#endif
        }
    }

    /*  We don't set iface_descr here because doing so requires
     *  capture_ui_utils.c which requires epan/prefs.c which is
     *  probably a bit too much dependency for here...
     */
    interface_opts.descr = g_strdup(capture_opts->default_options.descr);
    interface_opts.cfilter = g_strdup(capture_opts->default_options.cfilter);
    interface_opts.snaplen = capture_opts->default_options.snaplen;
    interface_opts.has_snaplen = capture_opts->default_options.has_snaplen;
    interface_opts.linktype = capture_opts->default_options.linktype;
    interface_opts.promisc_mode = capture_opts->default_options.promisc_mode;
#ifdef HAVE_EXTCAP
    interface_opts.extcap_fifo = g_strdup(capture_opts->default_options.extcap_fifo);
    interface_opts.extcap_args = NULL;
    interface_opts.extcap_pid = INVALID_EXTCAP_PID;
    interface_opts.extcap_userdata = NULL;
#endif
#ifdef CAN_SET_CAPTURE_BUFFER_SIZE
    interface_opts.buffer_size = capture_opts->default_options.buffer_size;
#endif
    interface_opts.monitor_mode = capture_opts->default_options.monitor_mode;
#ifdef HAVE_PCAP_REMOTE
    interface_opts.src_type = capture_opts->default_options.src_type;
    interface_opts.remote_host = g_strdup(capture_opts->default_options.remote_host);
    interface_opts.remote_port = g_strdup(capture_opts->default_options.remote_port);
    interface_opts.auth_type = capture_opts->default_options.auth_type;
    interface_opts.auth_username = g_strdup(capture_opts->default_options.auth_username);
    interface_opts.auth_password = g_strdup(capture_opts->default_options.auth_password);
    interface_opts.datatx_udp = capture_opts->default_options.datatx_udp;
    interface_opts.nocap_rpcap = capture_opts->default_options.nocap_rpcap;
    interface_opts.nocap_local = capture_opts->default_options.nocap_local;
#endif
#ifdef HAVE_PCAP_SETSAMPLING
    interface_opts.sampling_method = capture_opts->default_options.sampling_method;
    interface_opts.sampling_param  = capture_opts->default_options.sampling_param;
#endif

    g_array_append_val(capture_opts->ifaces, interface_opts);

    return 0;
}


int
capture_opts_add_opt(capture_options *capture_opts, int opt, const char *optarg_str_p, gboolean *start_capture)
{
    int status, snaplen;

    switch(opt) {
    case LONGOPT_NUM_CAP_COMMENT:  /* capture comment */
        if (capture_opts->capture_comment) {
            cmdarg_err("--capture-comment can be set only once per file");
            return 1;
        }
        capture_opts->capture_comment = g_strdup(optarg_str_p);
        break;
    case 'a':        /* autostop criteria */
        if (set_autostop_criterion(capture_opts, optarg_str_p) == FALSE) {
            cmdarg_err("Invalid or unknown -a flag \"%s\"", optarg_str_p);
            return 1;
        }
        break;
#ifdef HAVE_PCAP_REMOTE
    case 'A':
        if (get_auth_arguments(capture_opts, optarg_str_p) == FALSE) {
            cmdarg_err("Invalid or unknown -A arg \"%s\"", optarg_str_p);
            return 1;
        }
        break;
#endif
    case 'b':        /* Ringbuffer option */
        capture_opts->multi_files_on = TRUE;
        if (get_ring_arguments(capture_opts, optarg_str_p) == FALSE) {
            cmdarg_err("Invalid or unknown -b arg \"%s\"", optarg_str_p);
            return 1;
        }
        break;
#ifdef CAN_SET_CAPTURE_BUFFER_SIZE
    case 'B':        /* Buffer size */
        if (capture_opts->ifaces->len > 0) {
            interface_options interface_opts;

            interface_opts = g_array_index(capture_opts->ifaces, interface_options, capture_opts->ifaces->len - 1);
            capture_opts->ifaces = g_array_remove_index(capture_opts->ifaces, capture_opts->ifaces->len - 1);
            interface_opts.buffer_size = get_positive_int(optarg_str_p, "buffer size");
            g_array_append_val(capture_opts->ifaces, interface_opts);
        } else {
            capture_opts->default_options.buffer_size = get_positive_int(optarg_str_p, "buffer size");
        }
        break;
#endif
    case 'c':        /* Capture n packets */
        capture_opts->has_autostop_packets = TRUE;
        capture_opts->autostop_packets = get_positive_int(optarg_str_p, "packet count");
        break;
    case 'f':        /* capture filter */
        get_filter_arguments(capture_opts, optarg_str_p);
        break;
    case 'g':        /* enable group read access on the capture file(s) */
        capture_opts->group_read_access = TRUE;
        break;
    case 'H':        /* Hide capture info dialog box */
        capture_opts->show_info = FALSE;
        break;
    case 'i':        /* Use interface x */
        status = capture_opts_add_iface_opt(capture_opts, optarg_str_p);
        if (status != 0) {
            return status;
        }
        break;
#ifdef HAVE_PCAP_CREATE
    case 'I':        /* Capture in monitor mode */
        if (capture_opts->ifaces->len > 0) {
            interface_options interface_opts;

            interface_opts = g_array_index(capture_opts->ifaces, interface_options, capture_opts->ifaces->len - 1);
            capture_opts->ifaces = g_array_remove_index(capture_opts->ifaces, capture_opts->ifaces->len - 1);
            interface_opts.monitor_mode = TRUE;
            g_array_append_val(capture_opts->ifaces, interface_opts);
        } else {
            capture_opts->default_options.monitor_mode = TRUE;
        }
        break;
#endif
    case 'k':        /* Start capture immediately */
        *start_capture = TRUE;
        break;
    /*case 'l':*/    /* Automatic scrolling in live capture mode */
#ifdef HAVE_PCAP_SETSAMPLING
    case 'm':
        if (get_sampling_arguments(capture_opts, optarg_str_p) == FALSE) {
            cmdarg_err("Invalid or unknown -m arg \"%s\"", optarg_str_p);
            return 1;
        }
        break;
#endif
    case 'n':        /* Use pcapng format */
        capture_opts->use_pcapng = TRUE;
        break;
    case 'p':        /* Don't capture in promiscuous mode */
        if (capture_opts->ifaces->len > 0) {
            interface_options interface_opts;

            interface_opts = g_array_index(capture_opts->ifaces, interface_options, capture_opts->ifaces->len - 1);
            capture_opts->ifaces = g_array_remove_index(capture_opts->ifaces, capture_opts->ifaces->len - 1);
            interface_opts.promisc_mode = FALSE;
            g_array_append_val(capture_opts->ifaces, interface_opts);
        } else {
            capture_opts->default_options.promisc_mode = FALSE;
        }
        break;
    case 'P':        /* Use pcap format */
        capture_opts->use_pcapng = FALSE;
        break;
#ifdef HAVE_PCAP_REMOTE
    case 'r':
        if (capture_opts->ifaces->len > 0) {
            interface_options interface_opts;

            interface_opts = g_array_index(capture_opts->ifaces, interface_options, capture_opts->ifaces->len - 1);
            capture_opts->ifaces = g_array_remove_index(capture_opts->ifaces, capture_opts->ifaces->len - 1);
            interface_opts.nocap_rpcap = FALSE;
            g_array_append_val(capture_opts->ifaces, interface_opts);
        } else {
            capture_opts->default_options.nocap_rpcap = FALSE;
        }
        break;
#endif
    case 's':        /* Set the snapshot (capture) length */
        snaplen = get_natural_int(optarg_str_p, "snapshot length");
        /*
         * Make a snapshot length of 0 equivalent to the maximum packet
         * length, mirroring what tcpdump does.
         */
        if (snaplen == 0)
            snaplen = WTAP_MAX_PACKET_SIZE;
        if (capture_opts->ifaces->len > 0) {
            interface_options interface_opts;

            interface_opts = g_array_index(capture_opts->ifaces, interface_options, capture_opts->ifaces->len - 1);
            capture_opts->ifaces = g_array_remove_index(capture_opts->ifaces, capture_opts->ifaces->len - 1);
            interface_opts.has_snaplen = TRUE;
            interface_opts.snaplen = snaplen;
            g_array_append_val(capture_opts->ifaces, interface_opts);
        } else {
            capture_opts->default_options.snaplen = snaplen;
            capture_opts->default_options.has_snaplen = TRUE;
        }
        break;
    case 'S':        /* "Real-Time" mode: used for following file ala tail -f */
        capture_opts->real_time_mode = TRUE;
        break;
#ifdef HAVE_PCAP_REMOTE
    case 'u':
        if (capture_opts->ifaces->len > 0) {
            interface_options interface_opts;

            interface_opts = g_array_index(capture_opts->ifaces, interface_options, capture_opts->ifaces->len - 1);
            capture_opts->ifaces = g_array_remove_index(capture_opts->ifaces, capture_opts->ifaces->len - 1);
            interface_opts.datatx_udp = TRUE;
            g_array_append_val(capture_opts->ifaces, interface_opts);
        } else {
            capture_opts->default_options.datatx_udp = TRUE;
        }
        break;
#endif
    case 'w':        /* Write to capture file x */
        capture_opts->saving_to_file = TRUE;
        g_free(capture_opts->save_file);
        capture_opts->save_file = g_strdup(optarg_str_p);
        status = capture_opts_output_to_pipe(capture_opts->save_file, &capture_opts->output_to_pipe);
        return status;
    case 'y':        /* Set the pcap data link type */
        if (capture_opts->ifaces->len > 0) {
            interface_options interface_opts;

            interface_opts = g_array_index(capture_opts->ifaces, interface_options, capture_opts->ifaces->len - 1);
            capture_opts->ifaces = g_array_remove_index(capture_opts->ifaces, capture_opts->ifaces->len - 1);
            interface_opts.linktype = linktype_name_to_val(optarg_str_p);
            if (interface_opts.linktype == -1) {
                cmdarg_err("The specified data link type \"%s\" isn't valid",
                           optarg_str_p);
                return 1;
            }
            g_array_append_val(capture_opts->ifaces, interface_opts);
        } else {
            capture_opts->default_options.linktype = linktype_name_to_val(optarg_str_p);
            if (capture_opts->default_options.linktype == -1) {
                cmdarg_err("The specified data link type \"%s\" isn't valid",
                           optarg_str_p);
                return 1;
            }
        }
        break;
    default:
        /* the caller is responsible to send us only the right opt's */
        g_assert_not_reached();
    }

    return 0;
}

void
capture_opts_print_if_capabilities(if_capabilities_t *caps, char *name,
                                   gboolean monitor_mode)
{
    GList *lt_entry;
    data_link_info_t *data_link_info;

    if (caps->can_set_rfmon)
        printf("Data link types of interface %s when %sin monitor mode (use option -y to set):\n",
               name, monitor_mode ? "" : "not ");
    else
        printf("Data link types of interface %s (use option -y to set):\n", name);
    for (lt_entry = caps->data_link_types; lt_entry != NULL;
         lt_entry = g_list_next(lt_entry)) {
        data_link_info = (data_link_info_t *)lt_entry->data;
        printf("  %s", data_link_info->name);
        if (data_link_info->description != NULL)
            printf(" (%s)", data_link_info->description);
        else
            printf(" (not supported)");
        printf("\n");
    }
}

/* Print an ASCII-formatted list of interfaces. */
void
capture_opts_print_interfaces(GList *if_list)
{
    int         i;
    GList       *if_entry;
    if_info_t   *if_info;

    i = 1;  /* Interface id number */
    for (if_entry = g_list_first(if_list); if_entry != NULL;
         if_entry = g_list_next(if_entry)) {
        if_info = (if_info_t *)if_entry->data;
        printf("%d. %s", i++, if_info->name);

        /* Print the interface friendly name, if it exists;
          if not fall back to vendor description, if it exists. */
        if (if_info->friendly_name != NULL){
            printf(" (%s)", if_info->friendly_name);
        } else {
            if (if_info->vendor_description != NULL)
                printf(" (%s)", if_info->vendor_description);
        }
        printf("\n");
    }
}


void
capture_opts_trim_snaplen(capture_options *capture_opts, int snaplen_min)
{
    guint i;
    interface_options interface_opts;

    if (capture_opts->ifaces->len > 0) {
        for (i = 0; i < capture_opts->ifaces->len; i++) {
            interface_opts = g_array_index(capture_opts->ifaces, interface_options, 0);
            capture_opts->ifaces = g_array_remove_index(capture_opts->ifaces, 0);
            if (interface_opts.snaplen < 1)
                interface_opts.snaplen = WTAP_MAX_PACKET_SIZE;
            else if (interface_opts.snaplen < snaplen_min)
                interface_opts.snaplen = snaplen_min;
            g_array_append_val(capture_opts->ifaces, interface_opts);
        }
    } else {
        if (capture_opts->default_options.snaplen < 1)
            capture_opts->default_options.snaplen = WTAP_MAX_PACKET_SIZE;
        else if (capture_opts->default_options.snaplen < snaplen_min)
            capture_opts->default_options.snaplen = snaplen_min;
    }
}


void
capture_opts_trim_ring_num_files(capture_options *capture_opts)
{
    /* Check the value range of the ring_num_files parameter */
    if (capture_opts->ring_num_files > RINGBUFFER_MAX_NUM_FILES) {
        cmdarg_err("Too many ring buffer files (%u). Reducing to %u.\n", capture_opts->ring_num_files, RINGBUFFER_MAX_NUM_FILES);
        capture_opts->ring_num_files = RINGBUFFER_MAX_NUM_FILES;
    } else if (capture_opts->ring_num_files > RINGBUFFER_WARN_NUM_FILES) {
        cmdarg_err("%u is a lot of ring buffer files.\n", capture_opts->ring_num_files);
    }
#if RINGBUFFER_MIN_NUM_FILES > 0
    else if (capture_opts->ring_num_files < RINGBUFFER_MIN_NUM_FILES)
        cmdarg_err("Too few ring buffer files (%u). Increasing to %u.\n", capture_opts->ring_num_files, RINGBUFFER_MIN_NUM_FILES);
        capture_opts->ring_num_files = RINGBUFFER_MIN_NUM_FILES;
#endif
}

/*
 * If no interface was specified explicitly, pick a default.
 */
int
capture_opts_default_iface_if_necessary(capture_options *capture_opts,
                                        const char *capture_device)
{
    int status;

    /* Did the user specify an interface to use? */
    if (capture_opts->num_selected != 0 || capture_opts->ifaces->len != 0) {
        /* yes they did, return immediately - nothing further to do here */
        return 0;
    }

    /* No - is a default specified in the preferences file? */
    if (capture_device != NULL) {
        /* Yes - use it. */
        status = capture_opts_add_iface_opt(capture_opts, capture_device);
        return status;
    }
    /* No default in preferences file, just pick the first interface from the list of interfaces. */
    return capture_opts_add_iface_opt(capture_opts, "1");
}

#ifndef S_IFIFO
#define S_IFIFO _S_IFIFO
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(mode)  (((mode) & S_IFMT) == S_IFIFO)
#endif

/* copied from filesystem.c */
static int
capture_opts_test_for_fifo(const char *path)
{
    ws_statb64 statb;

    if (ws_stat64(path, &statb) < 0)
        return errno;

    if (S_ISFIFO(statb.st_mode))
        return ESPIPE;
    else
        return 0;
}

static gboolean
capture_opts_output_to_pipe(const char *save_file, gboolean *is_pipe)
{
    int err;

    *is_pipe = FALSE;

    if (save_file != NULL) {
        /* We're writing to a capture file. */
        if (strcmp(save_file, "-") == 0) {
            /* Writing to stdout. */
            /* XXX - should we check whether it's a pipe?  It's arguably
               silly to do "-w - >output_file" rather than "-w output_file",
               but by not checking we might be violating the Principle Of
               Least Astonishment. */
            *is_pipe = TRUE;
        } else {
            /* not writing to stdout, test for a FIFO (aka named pipe) */
            err = capture_opts_test_for_fifo(save_file);
            switch (err) {

            case ENOENT:      /* it doesn't exist, so we'll be creating it,
                                 and it won't be a FIFO */
            case 0:           /* found it, but it's not a FIFO */
                break;

            case ESPIPE:      /* it is a FIFO */
                *is_pipe = TRUE;
                break;

            default:          /* couldn't stat it              */
                break;          /* ignore: later attempt to open */
                /*  will generate a nice msg     */
            }
        }
    }

    return 0;
}

void
capture_opts_del_iface(capture_options *capture_opts, guint if_index)
{
    interface_options interface_opts;

    interface_opts = g_array_index(capture_opts->ifaces, interface_options, if_index);
    /* XXX - check if found? */

    g_free(interface_opts.name);
    g_free(interface_opts.descr);
    if (interface_opts.console_display_name != NULL)
        g_free(interface_opts.console_display_name);
    g_free(interface_opts.cfilter);
#ifdef HAVE_EXTCAP
    g_free(interface_opts.extcap);
    g_free(interface_opts.extcap_fifo);
    if (interface_opts.extcap_args)
        g_hash_table_unref(interface_opts.extcap_args);
    if (interface_opts.extcap_pid != INVALID_EXTCAP_PID)
        g_spawn_close_pid(interface_opts.extcap_pid);
    g_free(interface_opts.extcap_userdata);
#endif
#ifdef HAVE_PCAP_REMOTE
    if (interface_opts.src_type == CAPTURE_IFREMOTE) {
        g_free(interface_opts.remote_host);
        g_free(interface_opts.remote_port);
        g_free(interface_opts.auth_username);
        g_free(interface_opts.auth_password);
    }
#endif
    capture_opts->ifaces = g_array_remove_index(capture_opts->ifaces, if_index);
}



/*
 * Add all non-hidden selected interfaces in the "all interfaces" list
 * to the list of interfaces for the capture.
 */
void
collect_ifaces(capture_options *capture_opts)
{
    guint i;
    interface_t device;
    interface_options interface_opts;

    /* Empty out the existing list of interfaces. */
    for (i = capture_opts->ifaces->len; i != 0; i--)
        capture_opts_del_iface(capture_opts, i-1);

    /* Now fill the list up again. */
    for (i = 0; i < capture_opts->all_ifaces->len; i++) {
        device = g_array_index(capture_opts->all_ifaces, interface_t, i);
        if (!device.hidden && device.selected) {
            interface_opts.name = g_strdup(device.name);
            interface_opts.descr = g_strdup(device.display_name);
            interface_opts.console_display_name = g_strdup(device.name);
            interface_opts.linktype = device.active_dlt;
            interface_opts.cfilter = g_strdup(device.cfilter);
            interface_opts.snaplen = device.snaplen;
            interface_opts.has_snaplen = device.has_snaplen;
            interface_opts.promisc_mode = device.pmode;
            interface_opts.if_type = device.if_info.type;
#ifdef HAVE_EXTCAP
            interface_opts.extcap = g_strdup(device.if_info.extcap);
            interface_opts.extcap_fifo = NULL;
            interface_opts.extcap_userdata = NULL;
            interface_opts.extcap_args = device.external_cap_args_settings;
            interface_opts.extcap_pid = INVALID_EXTCAP_PID;
            if (interface_opts.extcap_args)
                g_hash_table_ref(interface_opts.extcap_args);
            interface_opts.extcap_userdata = NULL;
#endif
#ifdef CAN_SET_CAPTURE_BUFFER_SIZE
            interface_opts.buffer_size =  device.buffer;
#endif
#ifdef HAVE_PCAP_CREATE
            interface_opts.monitor_mode = device.monitor_mode_enabled;
#endif
#ifdef HAVE_PCAP_REMOTE
            interface_opts.src_type = CAPTURE_IFREMOTE;
            interface_opts.remote_host = g_strdup(device.remote_opts.remote_host_opts.remote_host);
            interface_opts.remote_port = g_strdup(device.remote_opts.remote_host_opts.remote_port);
            interface_opts.auth_type = device.remote_opts.remote_host_opts.auth_type;
            interface_opts.auth_username = g_strdup(device.remote_opts.remote_host_opts.auth_username);
            interface_opts.auth_password = g_strdup(device.remote_opts.remote_host_opts.auth_password);
            interface_opts.datatx_udp = device.remote_opts.remote_host_opts.datatx_udp;
            interface_opts.nocap_rpcap = device.remote_opts.remote_host_opts.nocap_rpcap;
            interface_opts.nocap_local = device.remote_opts.remote_host_opts.nocap_local;
#endif
#ifdef HAVE_PCAP_SETSAMPLING
            interface_opts.sampling_method = device.remote_opts.sampling_method;
            interface_opts.sampling_param  = device.remote_opts.sampling_param;
#endif
            g_array_append_val(capture_opts->ifaces, interface_opts);
        } else {
            continue;
        }
    }
}

static void
capture_opts_free_interface_t_links(gpointer elem, gpointer unused _U_)
{
    link_row* e = (link_row*)elem;
    if (e != NULL)
        g_free(e->name);
    g_free(elem);
}

static void
capture_opts_free_interface_t_addrs(gpointer elem, gpointer unused _U_)
{
    g_free(elem);
}

void
capture_opts_free_interface_t(interface_t *device)
{
    if (device != NULL) {
        g_free(device->name);
        g_free(device->display_name);
        g_free(device->friendly_name);
        g_free(device->addresses);
        g_free(device->cfilter);
        g_list_foreach(device->links,
                       capture_opts_free_interface_t_links, NULL);
        g_list_free(device->links);
#ifdef HAVE_PCAP_REMOTE
        g_free(device->remote_opts.remote_host_opts.remote_host);
        g_free(device->remote_opts.remote_host_opts.remote_port);
        g_free(device->remote_opts.remote_host_opts.auth_username);
        g_free(device->remote_opts.remote_host_opts.auth_password);
#endif
        g_free(device->if_info.name);
        g_free(device->if_info.friendly_name);
        g_free(device->if_info.vendor_description);
        g_slist_foreach(device->if_info.addrs,
                        capture_opts_free_interface_t_addrs, NULL);
        g_slist_free(device->if_info.addrs);
#ifdef HAVE_EXTCAP
        g_free(device->if_info.extcap);
#endif
    }
}

#endif /* HAVE_LIBPCAP */

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
