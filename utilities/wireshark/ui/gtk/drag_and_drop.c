/* drag_and_drop.c
 * Drag and Drop
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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>


#include "../../file.h"
#ifdef HAVE_LIBPCAP
#include "ui/capture.h"
#endif

#ifdef HAVE_LIBPCAP
#include "ui/capture_globals.h"
#endif
#include "ui/recent_utils.h"
#include "ui/simple_dialog.h"

#include "ui/gtk/gtkglobals.h"
#include "ui/gtk/capture_file_dlg.h"
#include "ui/gtk/drag_and_drop.h"
#include "ui/gtk/main.h"

#include "ui/gtk/old-gtk-compat.h"

#ifdef HAVE_GTKOSXAPPLICATION
#include <gtkmacintegration/gtkosxapplication.h>
#endif

enum { DND_TARGET_STRING, DND_TARGET_ROOTWIN, DND_TARGET_URL };

/* convert drag and drop URI to a local filename */
static gchar *
dnd_uri2filename(gchar *cf_name)
{
    gchar     *src, *dest;
    gint      ret;
    guint     i;
    gchar     esc[3];


    /* Remove URI header.
     * we have to remove the prefix to get a valid filename. */
#ifdef _WIN32
    /*
     * On win32 (at least WinXP), this prefix looks like (UNC):
     * file:////servername/sharename/dir1/dir2/capture-file.cap
     * or (local filename):
     * file:///d:/dir1/dir2/capture-file.cap
     */
    if (strncmp("file:////", cf_name, 9) == 0) {
        /* win32 UNC: now becoming: //servername/sharename/dir1/dir2/capture-file.cap */
        cf_name += 7;
    } else if (strncmp("file:///", cf_name, 8) == 0) {
        /* win32 local: now becoming: d:/dir1/dir2/capture-file.cap */
        cf_name += 8;
    }
#else
    /*
     * On UNIX (at least KDE 3.0 Konqueror), this prefix looks like:
     * file:/dir1/dir2/capture-file.cap
     *
     * On UNIX (at least GNOME Nautilus 2.8.2), this prefix looks like:
     * file:///dir1/dir2/capture-file.cap
     */
    if (strncmp("file:", cf_name, 5) == 0) {
        /* now becoming: /dir1/dir2/capture-file.cap or ///dir1/dir2/capture-file.cap */
        cf_name += 5;
        /* shorten //////thing to /thing */
        for(; cf_name[1] == '/'; ++cf_name);
    }
#endif

    /*
     * unescape the escaped URI characters (spaces, ...)
     *
     * we have to replace escaped chars to their equivalents,
     * e.g. %20 (always a two digit hexstring) -> ' '
     * the percent character '%' is escaped be a double one "%%"
     *
     * we do this conversation "in place" as the result is always
     * equal or smaller in size.
     */
    src = cf_name;
    dest = cf_name;
    while (*src) {
        if (*src == '%') {
            src++;
            if (*src == '%') {
                /* this is an escaped '%' char (was: "%%") */
                *dest = *src;
                src++;
                dest++;
            } else {
                /* convert escaped hexnumber to unscaped character */
                esc[0] = src[0];
                esc[1] = src[1];
                esc[2] = '\0';
                ret = sscanf(esc, "%x", &i);
                if (ret == 1) {
                    src+=2;
                    *dest = (gchar) i;
                    dest++;
                } else {
                    /* somethings wrong, just jump over that char
                     * this will result in a wrong string, but we might get
                     * user feedback and can fix it later ;-) */
                    src++;
                }
            }
#ifdef _WIN32
        } else if (*src == '/') {
            *dest = '\\';
            src++;
            dest++;
#endif
        } else {
            *dest = *src;
            src++;
            dest++;
        }
    }
    *dest = '\0';

    return cf_name;
}

/* open/merge the dnd file */
void
dnd_open_file_cmd(gchar *cf_names_freeme)
{
    int       err;
    gchar     *cf_name;
    int       in_file_count;
    int       files_work;
    char      **in_filenames;
    char      *tmpname;

    if (cf_names_freeme == NULL) return;

    /* DND_TARGET_URL:
     * The cf_name_freeme is a single string, containing one or more URI's,
     * terminated by CR/NL chars. The length of the whole field can be found
     * in the selection_data->length field. If it contains one file, simply open it,
     * If it contains more than one file, ask to merge these files. */

    /* count the number of input files */
    cf_name = cf_names_freeme;
    for(in_file_count = 0; (cf_name = strstr(cf_name, "\r\n")) != NULL; ) {
        cf_name += 2;
        in_file_count++;
    }
    if (in_file_count == 0) {
      g_free(cf_names_freeme);
      return;
    }

    in_filenames = (char **)g_malloc(sizeof(char*) * in_file_count);

    /* store the starts of the file entries in a gchar array */
    cf_name = cf_names_freeme;
    in_filenames[0] = cf_name;
    for(files_work = 1; (cf_name = strstr(cf_name, "\r\n")) != NULL && files_work < in_file_count; ) {
        cf_name += 2;
        in_filenames[files_work] = cf_name;
        files_work++;
    }

    /* replace trailing CR NL simply with zeroes (in place), so we get valid terminated strings */
    cf_name = cf_names_freeme;
    g_strdelimit(cf_name, "\r\n", '\0');

    /* convert all filenames from URI to local filename (in place) */
    for(files_work = 0; files_work < in_file_count; files_work++) {
        in_filenames[files_work] = dnd_uri2filename(in_filenames[files_work]);
    }

    if (in_file_count == 1) {
        /* open and read the capture file (this will close an existing file) */
        if (cf_open(&cfile, in_filenames[0], WTAP_TYPE_AUTO, FALSE, &err) == CF_OK) {
            /* XXX - add this to the menu if the read fails? */
            cf_read(&cfile, FALSE);
            add_menu_recent_capture_file(in_filenames[0]);
        } else {
            /* the capture file couldn't be read (doesn't exist, file format unknown, ...) */
        }
    } else {
        /* merge the files in chronological order */
        tmpname = NULL;
        if (cf_merge_files(&tmpname, in_file_count, in_filenames,
                           WTAP_FILE_TYPE_SUBTYPE_PCAPNG, FALSE) == CF_OK) {
            /* Merge succeeded; close the currently-open file and try
               to open the merged capture file. */
            cf_close(&cfile);
            if (cf_open(&cfile, tmpname, WTAP_TYPE_AUTO, TRUE /* temporary file */, &err) == CF_OK) {
                g_free(tmpname);
                cf_read(&cfile, FALSE);
            } else {
                /* The merged file couldn't be read. */
                g_free(tmpname);
            }
        } else {
            /* merge failed */
            g_free(tmpname);
        }
    }

    g_free(in_filenames);
    g_free(cf_names_freeme);
}

/* we have received some drag and drop data */
/* (as we only registered to "text/uri-list", we will only get a file list here) */
static void
dnd_data_received(GtkWidget *widget _U_, GdkDragContext *dc _U_, gint x _U_, gint y _U_,
                  GtkSelectionData *selection_data, guint info, guint t _U_, gpointer data _U_)
{
    gchar *cf_names_freeme;
    const guchar *sel_data_data;
    gint sel_data_len;

    if (info == DND_TARGET_URL) {
        /* Usually we block incoming events by disabling the corresponding menu/toolbar items.
         * This is the only place where an incoming event won't be blocked in such a way,
         * so we have to take care of NOT loading a new file while a different process
         * (e.g. capture/load/...) is still in progress. */

#ifdef HAVE_LIBPCAP
        /* if a capture is running, do nothing but warn the user */
        if((global_capture_session.state != CAPTURE_STOPPED)) {
            simple_dialog(ESD_TYPE_CONFIRMATION,
                        ESD_BTN_OK,
                        "%sDrag and Drop currently not possible!%s\n\n"
                        "Dropping a file isn't possible while a capture is in progress.",
                        simple_dialog_primary_start(), simple_dialog_primary_end());
            return;
        }
#endif

        /* if another file read is still in progress, do nothing but warn the user */
        if(cfile.state == FILE_READ_IN_PROGRESS) {
            simple_dialog(ESD_TYPE_CONFIRMATION,
                        ESD_BTN_OK,
                        "%sDrag and Drop currently not possible!%s\n\n"
                        "Dropping a file isn't possible while loading another capture file.",
                        simple_dialog_primary_start(), simple_dialog_primary_end());
            return;
        }

        /* the selection_data will soon be gone, make a copy first */
        /* the data string is not zero terminated -> make a zero terminated "copy" of it */
        sel_data_len = gtk_selection_data_get_length(selection_data);
        sel_data_data = gtk_selection_data_get_data(selection_data);
        cf_names_freeme = (gchar *)g_malloc(sel_data_len + 1);
        memcpy(cf_names_freeme, sel_data_data, sel_data_len);
        cf_names_freeme[sel_data_len] = '\0';

        /* If there's unsaved data, let the user save it first.
           If they cancel out of it, don't open the file. */
        if (do_file_close(&cfile, FALSE, " before opening a new capture file"))
            dnd_open_file_cmd(cf_names_freeme);
    }
}

#ifdef HAVE_GTKOSXAPPLICATION
gboolean
gtk_osx_openFile (GtkosxApplication *app _U_, gchar *path, gpointer user_data _U_)
{
    GtkSelectionData selection_data;
    gchar* selection_path;
    size_t length = strlen(path);

    selection_path = (gchar *)g_malloc(length + 3);
    memcpy(selection_path, path, length);

    selection_path[length] = '\r';
    selection_path[length + 1] = '\n';
    selection_path[length + 2] = '\0';

    memset(&selection_data, 0, sizeof(selection_data));

    gtk_selection_data_set(&selection_data, gdk_atom_intern_static_string ("text/uri-list"), 8, (guchar*) selection_path, (gint)(length + 2));
    dnd_data_received(NULL, NULL, 0, 0, &selection_data, DND_TARGET_URL, 0, 0);

    return TRUE;
}
#endif

/* init the drag and drop functionality */
void
dnd_init(GtkWidget *w)
{
    /* we are only interested in the URI list containing filenames */
    static GtkTargetEntry target_entry[] = {
         /*{"STRING", 0, DND_TARGET_STRING},*/
         /*{"text/plain", 0, DND_TARGET_STRING},*/
         {"text/uri-list", 0, DND_TARGET_URL}
    };

    /* set this window as a dnd destination */
    gtk_drag_dest_set(
         w, GTK_DEST_DEFAULT_ALL, target_entry,
         sizeof(target_entry) / sizeof(GtkTargetEntry),
         (GdkDragAction)(GDK_ACTION_MOVE | GDK_ACTION_COPY) );

    /* get notified, if some dnd coming in */
    g_signal_connect(w, "drag_data_received", G_CALLBACK(dnd_data_received), NULL);
#ifdef HAVE_GTKOSXAPPLICATION
    g_signal_connect(g_object_new(GTKOSX_TYPE_APPLICATION, NULL), "NSApplicationOpenFile", G_CALLBACK(gtk_osx_openFile), NULL);
#endif
}

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
