/* dlg_utils.c
 * Utilities to use when constructing dialogs
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

#include "ui/gtk/gtkglobals.h"
#include "ui/gtk/gui_utils.h"
#include "ui/gtk/dlg_utils.h"
#include "ui/gtk/stock_icons.h"
#include "ui/gtk/old-gtk-compat.h"

#include "wsutil/filesystem.h"


static void
dlg_activate (GtkWidget *widget, gpointer ok_button);

/* create a button for the button row (helper for dlg_button_row_new) */
static GtkWidget *
dlg_button_new(GtkWidget *hbox, GtkWidget *button_hbox, const gchar *stock_id)
{
    GtkWidget *button;

    button = ws_gtk_button_new_from_stock(stock_id);
    gtk_widget_set_can_default(button, TRUE);
    g_object_set_data(G_OBJECT(hbox), stock_id, button);
    gtk_box_pack_end(GTK_BOX(button_hbox), button, FALSE, FALSE, 0);
    gtk_widget_show(button);
    return button;
}

/*
 * Set the focus and default for the nth item in a button row, with
 * 0 being the first item.
 */
#define BUTTON_HBOX_KEY "button_hbox"
void
dlg_button_focus_nth(GtkWidget *hbox, gint focus_item) {
    GtkWidget *button_hbox, *button;
    GList     *children;
    gint       cur_item = 0;

    if (!hbox)
        return;

    button_hbox = (GtkWidget *)g_object_get_data(G_OBJECT(hbox), BUTTON_HBOX_KEY);
    children = gtk_container_get_children(GTK_CONTAINER(button_hbox));

    while (children) {
        if (cur_item == focus_item) {
            button = (GtkWidget *)children->data;
            gtk_widget_grab_focus(button);
            gtk_widget_grab_default(button);
            break;
        }
        children = g_list_next(children);
        cur_item++;
    }

    g_list_free(children);
}

/* create a button row for a dialog */

/* The purpose of this is, to have one place available, where all button rows
 * from all dialogs are laid out. This will:
 *
 * a.) keep the button layout more consistent over the different dialogs
 * b.) being able to switch between different button layouts, e.g.:
 *     e.g. Win32: "OK" "Apply" "Cancel"
 *     e.g. GNOME: "Apply" "Cancel" "OK"
 */
GtkWidget *
dlg_button_row_new(const gchar *stock_id_first, ...)
{
    gint         buttons  = 0;
    va_list      stock_id_list;
    const gchar *stock_id = stock_id_first;
    GtkWidget   *hbox;
    GtkWidget   *button_hbox;
    GtkWidget   *help_hbox;
    GtkWidget   *button;

    const gchar *apply         = NULL;
    const gchar *cancel        = NULL;
    const gchar *cap_start     = NULL;
    const gchar *cap_stop      = NULL;
    const gchar *cap_options   = NULL;
#ifdef _WIN32
    const gchar *cap_details   = NULL;
#endif
    const gchar *clear         = NULL;
    const gchar *closex        = NULL;
    const gchar *copy          = NULL;
    const gchar *create_stat   = NULL;
    const gchar *delete_id     = NULL;
    const gchar *dont_save     = NULL;
    const gchar *filter_stream = NULL;
    const gchar *find          = NULL;
    const gchar *help          = NULL;
    const gchar *jump          = NULL;
    const gchar *no            = NULL;
    const gchar *ok            = NULL;
    const gchar *print         = NULL;
    const gchar *save          = NULL;
    const gchar *save_as       = NULL;
    const gchar *save_all      = NULL;
    const gchar *stop          = NULL;
    const gchar *yes           = NULL;
    const gchar *refresh       = NULL;
    const gchar *add           = NULL;
#ifdef HAVE_GEOIP
    const gchar *map           = NULL;
#endif /* HAVE_GEOIP */
    const gchar *follow_stream = NULL;
    const gchar *graph_a_b     = NULL;
    const gchar *graph_b_a     = NULL;


    va_start(stock_id_list, stock_id_first);

    /* get all buttons needed */
    while(stock_id != NULL) {
        if (strcmp(stock_id, GTK_STOCK_OK) == 0) {
            ok = stock_id;
        } else if (strcmp(stock_id, WIRESHARK_STOCK_CREATE_STAT) == 0) {
            create_stat = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_APPLY) == 0) {
            apply = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_SAVE) == 0) {
            save = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_SAVE_AS) == 0) {
            save_as = stock_id;
        } else if (strcmp(stock_id, WIRESHARK_STOCK_SAVE_ALL) == 0) {
            save_all = stock_id;
        } else if (strcmp(stock_id, WIRESHARK_STOCK_DONT_SAVE) == 0) {
            dont_save = stock_id;
        } else if (strcmp(stock_id, WIRESHARK_STOCK_QUIT_DONT_SAVE) == 0) {
            dont_save = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_CANCEL) == 0) {
            cancel = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_CLOSE) == 0) {
            closex = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_CLEAR) == 0) {
            clear = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_REFRESH) == 0) {
            refresh = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_ADD) == 0) {
           add = stock_id;
#ifdef HAVE_LIBPCAP
        } else if (strcmp(stock_id, WIRESHARK_STOCK_CAPTURE_START) == 0) {
            cap_start = stock_id;
        } else if (strcmp(stock_id, WIRESHARK_STOCK_CAPTURE_STOP) == 0) {
            cap_stop = stock_id;
        } else if (strcmp(stock_id, WIRESHARK_STOCK_CAPTURE_OPTIONS) == 0) {
            cap_options = stock_id;
#ifdef _WIN32
        } else if (strcmp(stock_id, WIRESHARK_STOCK_CAPTURE_DETAILS) == 0) {
            cap_details = stock_id;
#endif
#endif /* HAVE_LIBPCAP */
#ifdef HAVE_GEOIP
        } else if (strcmp(stock_id, WIRESHARK_STOCK_MAP) == 0) {
            map = stock_id;
#endif /* HAVE_GEOIP */
        } else if (strcmp(stock_id, WIRESHARK_STOCK_FOLLOW_STREAM) == 0) {
            follow_stream = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_STOP) == 0) {
            stop = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_HELP) == 0) {
            help = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_PRINT) == 0) {
            print = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_FIND) == 0) {
            find = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_JUMP_TO) == 0) {
            jump = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_YES) == 0) {
            yes = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_NO) == 0) {
            no = stock_id;
        } else if (strcmp(stock_id, WIRESHARK_STOCK_FILTER_OUT_STREAM) == 0) {
            filter_stream = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_DELETE) == 0) {
            delete_id = stock_id;
        } else if (strcmp(stock_id, GTK_STOCK_COPY) == 0) {
            copy = stock_id;
        } else if (strcmp(stock_id, WIRESHARK_STOCK_GRAPH_A_B) == 0) {
            graph_a_b = stock_id;
        } else if (strcmp(stock_id, WIRESHARK_STOCK_GRAPH_B_A) == 0) {
            graph_b_a = stock_id;
        } else {
            /* we don't know that button! */
            g_assert_not_reached();
        }
        buttons++;
        stock_id = va_arg(stock_id_list, gchar *);
    }
    va_end(stock_id_list);

    hbox = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, DLG_BUTTON_SPACING, FALSE);
    gtk_widget_show(hbox);

    button_hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_end(GTK_BOX(hbox), button_hbox, TRUE, TRUE, 0);
    g_object_set_data(G_OBJECT(hbox), BUTTON_HBOX_KEY, button_hbox);
    gtk_widget_show(button_hbox);
    gtk_box_set_spacing(GTK_BOX(button_hbox), DLG_BUTTON_SPACING);

    help_hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_end(GTK_BOX(hbox), help_hbox, FALSE, FALSE, 0);
    gtk_widget_show(help_hbox);
    gtk_box_set_spacing(GTK_BOX(help_hbox), DLG_BUTTON_SPACING);

    if (buttons == 0) {
        /* if no buttons wanted, simply do nothing */
        return hbox;
    }

    if (buttons == 1) {
        /* if only one button, simply put it in the middle (default) */
        dlg_button_new(hbox, button_hbox, stock_id_first);
        return hbox;
    }

    /* do we have a help button? -> special handling for it */
    if (help) {
        button = ws_gtk_button_new_from_stock(help);
        gtk_widget_set_can_default(button, TRUE);
        g_object_set_data(G_OBJECT(hbox), help, button);
        gtk_box_pack_start(GTK_BOX(help_hbox), button, FALSE, FALSE, 0);
        gtk_widget_show(button);
        buttons--;
    }

    /* do we have a copy button? -> special handling for it */
    if (copy) {
        button = ws_gtk_button_new_from_stock(copy);
        gtk_widget_set_can_default(button, TRUE);
        g_object_set_data(G_OBJECT(hbox), copy, button);
        gtk_box_pack_start(GTK_BOX(help_hbox), button, FALSE, FALSE, 0);
        gtk_widget_show(button);
        buttons--;
    }

    /* do we have a refresh button? -> special handling for it */
    if (refresh) {
        button = ws_gtk_button_new_from_stock(refresh);
        g_object_set_data(G_OBJECT(hbox), refresh, button);
        gtk_box_pack_start(GTK_BOX(help_hbox), button, FALSE, FALSE, 0);
        gtk_widget_show(button);
        buttons--;
    }

    /* do we have an add button? -> special handling for it */
    if (add) {
        button = ws_gtk_button_new_from_stock(add);
        g_object_set_data(G_OBJECT(hbox), add, button);
        gtk_box_pack_start(GTK_BOX(help_hbox), button, FALSE, FALSE, 0);
        gtk_widget_show(button);
        buttons--;
    }

#ifdef HAVE_GEOIP
    /* do we have a map button? -> special handling for it */
    if (map) {
        button = ws_gtk_button_new_from_stock(map);
        gtk_widget_set_can_default(button, TRUE);
        g_object_set_data(G_OBJECT(hbox), map, button);
        gtk_box_pack_start(GTK_BOX(help_hbox), button, FALSE, FALSE, 0);
        gtk_widget_show(button);
        buttons--;
    }
#endif /* HAVE_GEOIP */

    /* if more than one button, sort buttons from left to right */
    /* (the whole button cluster will then be right aligned) */
    gtk_button_box_set_layout (GTK_BUTTON_BOX(button_hbox), GTK_BUTTONBOX_END);

#if !defined(_WIN32)
    /* beware: sequence of buttons are important! */

    /* XXX: this can be implemented more elegant of course, but it works as it should */
    if (buttons == 2) {
        if (ok && cancel) {
            dlg_button_new(hbox, button_hbox, cancel);
            dlg_button_new(hbox, button_hbox, ok);
            return hbox;
        }
        if (print && cancel) {
            dlg_button_new(hbox, button_hbox, cancel);
            dlg_button_new(hbox, button_hbox, print);
            return hbox;
        }
        if (find && cancel) {
            dlg_button_new(hbox, button_hbox, cancel);
            dlg_button_new(hbox, button_hbox, find);
            return hbox;
        }
        if (jump && cancel) {
            dlg_button_new(hbox, button_hbox, cancel);
            dlg_button_new(hbox, button_hbox, jump);
            return hbox;
        }
        if (save && cancel) {
            dlg_button_new(hbox, button_hbox, cancel);
            dlg_button_new(hbox, button_hbox, save);
            return hbox;
        }
        if (ok && clear) {
            dlg_button_new(hbox, button_hbox, clear);
            dlg_button_new(hbox, button_hbox, ok);
            return hbox;
        }
        if (save && closex) {
            dlg_button_new(hbox, button_hbox, closex);
            dlg_button_new(hbox, button_hbox, save);
            return hbox;
        }
        if (create_stat && cancel) {
            dlg_button_new(hbox, button_hbox, cancel);
            dlg_button_new(hbox, button_hbox, create_stat);
            return hbox;
        }
        if (cap_start && cancel) {
            dlg_button_new(hbox, button_hbox, cancel);
            dlg_button_new(hbox, button_hbox, cap_start);
            return hbox;
        }
        if (cap_stop && cancel) {
            dlg_button_new(hbox, button_hbox, cancel);
            dlg_button_new(hbox, button_hbox, cap_stop);
            return hbox;
        }
        if (delete_id && cancel) {
            dlg_button_new(hbox, button_hbox, cancel);
            dlg_button_new(hbox, button_hbox, delete_id);
            return hbox;
        }
    }
    if (buttons == 3) {
        if (ok && save && closex) {
            dlg_button_new(hbox, button_hbox, save);
            dlg_button_new(hbox, button_hbox, closex);
            dlg_button_new(hbox, button_hbox, ok);
            return hbox;
        }
        if (ok && apply && cancel) {
            dlg_button_new(hbox, button_hbox, apply);
            dlg_button_new(hbox, button_hbox, cancel);
            dlg_button_new(hbox, button_hbox, ok);
            return hbox;
        }
        if (apply && save && closex) {
            dlg_button_new(hbox, button_hbox, save);
            dlg_button_new(hbox, button_hbox, closex);
            dlg_button_new(hbox, button_hbox, apply);
            return hbox;
        }
        if (yes && no && cancel) {
            dlg_button_new(hbox, button_hbox, no);
            dlg_button_new(hbox, button_hbox, cancel);
            dlg_button_new(hbox, button_hbox, yes);
            return hbox;
        }
        if (save && dont_save && cancel) {
            dlg_button_new(hbox, button_hbox, dont_save);
            dlg_button_new(hbox, button_hbox, cancel);
            dlg_button_new(hbox, button_hbox, save);
            return hbox;
        }
    }
    if (buttons == 4) {
        if (ok && apply && save && cancel) {
            dlg_button_new(hbox, button_hbox, save);
            dlg_button_new(hbox, button_hbox, apply);
            dlg_button_new(hbox, button_hbox, cancel);
            dlg_button_new(hbox, button_hbox, ok);
            return hbox;
        }
        if (ok && apply && save && closex) {
            dlg_button_new(hbox, button_hbox, save);
            dlg_button_new(hbox, button_hbox, apply);
            dlg_button_new(hbox, button_hbox, closex);
            dlg_button_new(hbox, button_hbox, ok);
            return hbox;
        }
    }
#endif

    /* beware: sequence of buttons is important! */
    if (ok            != NULL) dlg_button_new(hbox, button_hbox, ok);
    if (delete_id     != NULL) dlg_button_new(hbox, button_hbox, delete_id);
    if (jump          != NULL) dlg_button_new(hbox, button_hbox, jump);
    if (find          != NULL) dlg_button_new(hbox, button_hbox, find);
    if (print         != NULL) dlg_button_new(hbox, button_hbox, print);
    if (create_stat   != NULL) dlg_button_new(hbox, button_hbox, create_stat);
    if (apply         != NULL) dlg_button_new(hbox, button_hbox, apply);
    if (yes           != NULL) dlg_button_new(hbox, button_hbox, yes);
    if (no            != NULL) dlg_button_new(hbox, button_hbox, no);
    if (save          != NULL) dlg_button_new(hbox, button_hbox, save);
    if (save_as       != NULL) dlg_button_new(hbox, button_hbox, save_as);
    if (save_all      != NULL) dlg_button_new(hbox, button_hbox, save_all);
    if (dont_save     != NULL) dlg_button_new(hbox, button_hbox, dont_save);
    if (cap_start     != NULL) dlg_button_new(hbox, button_hbox, cap_start);
    if (cap_stop      != NULL) dlg_button_new(hbox, button_hbox, cap_stop);
    if (cap_options   != NULL) dlg_button_new(hbox, button_hbox, cap_options);
#ifdef _WIN32
    if (cap_details   != NULL) dlg_button_new(hbox, button_hbox, cap_details);
#endif
    if (stop          != NULL) dlg_button_new(hbox, button_hbox, stop);
    if (clear         != NULL) dlg_button_new(hbox, button_hbox, clear);
    if (filter_stream != NULL) dlg_button_new(hbox, button_hbox, filter_stream);
    if (follow_stream != NULL) dlg_button_new(hbox, button_hbox, follow_stream);
    if (graph_a_b     != NULL) dlg_button_new(hbox, button_hbox, graph_a_b);
    if (graph_b_a     != NULL) dlg_button_new(hbox, button_hbox, graph_b_a);
    if (closex        != NULL) dlg_button_new(hbox, button_hbox, closex);
    if (cancel        != NULL) dlg_button_new(hbox, button_hbox, cancel);

    return hbox;
}


/* Create a dialog box window that belongs to Wireshark's main window. */
GtkWidget *
dlg_window_new(const gchar *title)
{
    GtkWidget *win;

    win = window_new(GTK_WINDOW_TOPLEVEL, title);

    /*
     * On Windows, making the dialogs transient to top_level behaves strangely.
     * It is not possible any more to bring the top level window to front easily.
     * So we don't do this on Windows.
     *
     * XXX: Note well: This means that *on Windows* any code which creates a
     *      window using dlg_window_new() and then calls
     *      gtk_widget_destroy_with_parent() will *not* get the desired effect
     *      since the dialog window actually has has no parent.
     */
#ifndef _WIN32
    if (top_level) {
        gtk_window_set_transient_for(GTK_WINDOW(win), GTK_WINDOW(top_level));
    }
#endif /*_WIN32*/

    return win;
}

/* Create a dialog box window that belongs to Wireshark's main window. */
GtkWidget *
dlg_window_new_with_geom(const gchar *title, const gchar *geom_name, GtkWindowPosition pos)
{
    GtkWidget *win;

    win = window_new_with_geom(GTK_WINDOW_TOPLEVEL, title, geom_name ? geom_name : title, pos);

    /*
     * XXX - if we're running in the capture child process, we can't easily
     * make this window transient for the main process's window.  We just
     * punt here.
     *
     * Perhaps the child process should only capture packets, write them to
     * a file, and somehow notify the parent process and let *it* do all
     * the GUI work.  If we can do that efficiently (so that we don't drop
     * more packets), perhaps we can also do so even when we're *not* doing
     * an "Update list of packets in real time" capture.  That'd let the
     * child process run set-UID on platforms where you need that in order
     * to capture, and might also simplify the job of having the GUI main
     * loop wait both for user input and packet arrival.
     */
    /*
     * On Windows, making the dialogs transient to top_level behaves strangely.
     * It is not possible any more to bring the top level window to front easily.
     * So we don't do this on Windows.
     */
#ifndef _WIN32
    if (top_level) {
        gtk_window_set_transient_for(GTK_WINDOW(win), GTK_WINDOW(top_level));
    }
#endif /*_WIN32*/

    return win;
}

/* Create a configuration dialog box window that belongs to Wireshark's
 * main window and add the name of the current profile name to its title bar
 */
GtkWidget *
dlg_conf_window_new(const gchar *title)
{
    const char *profile_name;
    gchar      *win_name;
    GtkWidget  *win;

    /*
     * Set window title to reflect which preferences profile we are
     * working with.
     */
    profile_name = get_profile_name();

    win_name = g_strdup_printf("%s - Profile: %s", title, profile_name);
    win = dlg_window_new(win_name);

    g_free(win_name);

    return win;
}

/*  Set the "activate" signal for a widget to call a routine to
    activate the "OK" button for a dialog box.

    XXX - there should be a way to specify that a GtkEntry widget
    shouldn't itself handle the Return key, but should let it be
    passed on to the parent, so that you don't have to do this
    by hand for every GtkEntry widget in a dialog box, but, alas,
    there isn't.  (Does this problem exist for other widgets?
    I.e., are there any others that seize the Return key? */
void
dlg_set_activate(GtkWidget *widget, GtkWidget *ok_button)
{
    g_signal_connect(widget, "activate", G_CALLBACK(dlg_activate), ok_button);
}

static void
dlg_activate (GtkWidget *widget _U_, gpointer ok_button)
{
    gtk_widget_activate(GTK_WIDGET(ok_button));
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
