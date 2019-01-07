/* gui_utils.c
 * UI utility routines, some GTK+-specific (declared in gtk/gui_utils.h)
 * and some with GUI-independent APIs, with this file containing the GTK+
 * implementations of them (declared in ui_util.h)
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

#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if GTK_CHECK_VERSION(3,0,0)
# include <gdk/gdkkeysyms-compat.h>
#endif

#include <epan/prefs.h>
#include "epan/epan.h"

#include <epan/packet_info.h>

#include "../../globals.h"

#include "ui/recent.h"
#include "ui/ui_util.h"

#include <wsutil/file_util.h>

#include "ui/gtk/gtkglobals.h"
#include "ui/gtk/gui_utils.h"
#include "ui/gtk/font_utils.h"
#include "ui/gtk/color_utils.h"
#include "ui/gtk/old-gtk-compat.h"
#ifndef HAVE_GDK_GRESOURCE
#include "ui/gtk/pixbuf-csource.h"
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#define WINDOW_GEOM_KEY "window_geom"
#define WINDOW_DECORATION_SIZE 50

/* Set our window icon.  The GDK documentation doesn't provide any
   actual documentation for gdk_window_set_icon(), so we'll steal
   libgimp/gimpdialog.c:gimp_dialog_realize_callback() from the Gimp
   sources and assume it's safe.

   XXX - The current icon size is fixed at 16x16 pixels, which looks fine
   with kwm (KDE 1.x's window manager), Sawfish (the "default" window
   manager for GNOME?), and under Windows with Exceed putting X windows
   on the Windows desktop, using Exceed as the window manager, as those
   window managers put a 16x16 icon on the title bar.

   The window managers in some windowing environments (e.g. dtwm in CDE)
   and some stand-alone window managers have larger icon sizes (many window
   managers put the window icon on the desktop, in the Windows 3.x style,
   rather than in the titlebar, in the Windows 4.x style), so we need to
   find a way to size our icon appropriately.

   The X11 Inter-Client Communications Conventions Manual, Version 1.1,
   in X11R5, specifies that "a window manager that wishes to place
   constraints on the sizes of icon pixmaps and/or windows should
   place a property called WM_ICON_SIZE on the root"; that property
   contains minimum width and height, maximum width and height, and
   width and height increment values.  "XGetIconSizes()" retrieves
   that property; unfortunately, I've yet to find a window manager
   that sets it on the root window (kwm, AfterStep, and Exceed don't
   appear to set it).

   The X Desktop Group's Window Manager Standard specifies, in the section
   on Application Window Properties, an _NET_WM_ICON property, presumably
   set by the window manager, which is an array of possible icon sizes
   for the client.  There's no API in GTK+ 1.2[.x] for this; there may
   eventually be one either in GTK+ 2.0 or GNOME 2.0.

   Some window managers can be configured to take the window name
   specified by the WM_NAME property of a window or the resource
   or class name specified by the WM_CLASS property and base the
   choice of icon for the window on one of those; WM_CLASS for
   Wireshark's windows has a resource name of "wireshark" and a class
   name of "Wireshark".  However, the way that's done is window-manager-
   specific, and there's no way to determine what size a particular
   window manager would want, so there's no way to automate this as
   part of the installation of Wireshark.
   */
static void
window_icon_realize_cb(GtkWidget *win,
                       gpointer   data _U_)
{
#ifndef _WIN32
    GList     *ws_icon_list = NULL;
    GdkPixbuf *icon16, *icon32, *icon48, *icon64;

#ifdef HAVE_GDK_GRESOURCE
    icon16 = ws_gdk_pixbuf_new_from_resource("/org/wireshark/image/wsicon16.png");
    icon32 = ws_gdk_pixbuf_new_from_resource("/org/wireshark/image/wsicon32.png");
    icon48 = ws_gdk_pixbuf_new_from_resource("/org/wireshark/image/wsicon48.png");
    icon64 = ws_gdk_pixbuf_new_from_resource("/org/wireshark/image/wsicon64.png");
#else
    icon16 = gdk_pixbuf_new_from_inline(-1, wsicon_16_pb_data, FALSE, NULL);
    icon32 = gdk_pixbuf_new_from_inline(-1, wsicon_32_pb_data, FALSE, NULL);
    icon48 = gdk_pixbuf_new_from_inline(-1, wsicon_48_pb_data, FALSE, NULL);
    icon64 = gdk_pixbuf_new_from_inline(-1, wsicon_64_pb_data, FALSE, NULL);
#endif
    ws_icon_list = g_list_append(ws_icon_list, icon16);
    ws_icon_list = g_list_append(ws_icon_list, icon32);
    ws_icon_list = g_list_append(ws_icon_list, icon48);
    ws_icon_list = g_list_append(ws_icon_list, icon64);

    gtk_window_set_icon_list(GTK_WINDOW(win), ws_icon_list);

    g_list_foreach(ws_icon_list, (GFunc)g_object_unref, NULL);
    g_list_free(ws_icon_list);

    /* set icon by name, this allows us to use even SVG icon if it is present */
    gtk_window_set_icon_name(GTK_WINDOW(win), "wireshark");
#endif
}


/* Create a new window, of the specified type, with the specified title
   (if any) and the Wireshark icon. */
GtkWidget *
window_new(GtkWindowType  type,
           const gchar   *title)
{
    GtkWidget *win;

    win = gtk_window_new(type);
    if (title != NULL)
        gtk_window_set_title(GTK_WINDOW(win), title);
    g_signal_connect(win, "realize", G_CALLBACK(window_icon_realize_cb), NULL);

    /* XXX - which one is the correct default policy? or use a preference for this? */
    /* GTK_WIN_POS_NONE, GTK_WIN_POS_CENTER or GTK_WIN_POS_MOUSE */
    /* a lot of people dislike GTK_WIN_POS_MOUSE */

    /* set the initial position (must be done, before show is called!) */
    gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_NONE);

    if (top_level) {
        GdkScreen *default_screen;
        gint x, y, n;

        /* Ideally, new windows would open on the same monitor where the main
         * window is located, but this doesn't happen when the main window is
         * not located on the primary monitor.  So, if there's more than 1
         * monitor and Wireshark's main window isn't located on the primary
         * one, attempt to improve the situation by at least displaying the new
         * window somewhere on the same monitor, even if it won't be positioned
         * the same way as it would be when it's on the primary monitor.  Don't
         * attempt to influence the placement on the primary monitor though,
         * because that's probably the preferred placement strategy.  But how
         * to make window placement behave the same way on any monitor?
         */
        default_screen = gdk_screen_get_default();
        n = gdk_screen_get_n_monitors(default_screen);
        if (n > 1) {
            gtk_window_get_position(GTK_WINDOW(top_level), &x, &y);
            n = gdk_screen_get_monitor_at_point(default_screen, x, y);
            if (n > 0)
                gtk_window_move(GTK_WINDOW(win), x + 40, y + 30);
        }
    }

    return win;
}


/* Same as window_new(), but will keep its geometry values (size, position, ...).
 * Be sure to use window_present() and window_destroy() appropriately! */
GtkWidget *
window_new_with_geom(GtkWindowType  type,
                     const gchar   *title,
                     const gchar   *geom_name,
                     GtkWindowPosition pos)
{
    window_geometry_t geom;
    GtkWidget *win = window_new(type, title);

    g_object_set_data(G_OBJECT(win), WINDOW_GEOM_KEY, (gpointer)g_strdup(geom_name));

    /* do we have a previously saved size and position of this window? */
    if(geom_name) {
        /* It's a good idea to set the position and size of the window already here,
         * as it's still invisible and won't "flicker the screen" while initially resizing. */
        if(window_geom_load(geom_name, &geom)) {
            /* XXX - use prefs to select which values to set? */
            geom.set_pos        = TRUE;
            geom.set_size       = TRUE;
            geom.set_maximized  = FALSE;  /* don't maximize until window is shown */
            window_set_geometry(win, &geom);
        } else if (pos != GTK_WIN_POS_NONE) {
#ifdef _WIN32
            /* Testing using GTK+ 2.24.10 shows that
             * GTK_WIN_POS_CENTER_ON_PARENT doesn't seem to work on Windows, so
             * use the next best thing.  Is this a problem for all OS's though,
             * or just Windows?  Unknown. (Tested with Windows XP SP3 32-bit)
             */
            if (pos == GTK_WIN_POS_CENTER_ON_PARENT)
                pos = GTK_WIN_POS_CENTER;
#endif
            gtk_window_set_position(GTK_WINDOW(win), pos);
        }
    }

    return win;
}


/* Create a new window for a splash screen; it's a main window, without decoration,
   positioned in the center of the screen. */
GtkWidget *
splash_window_new(void)
{
    GtkWidget *win;

    win = window_new(GTK_WINDOW_TOPLEVEL, "Wireshark");
    gtk_window_set_decorated(GTK_WINDOW(win), FALSE);

    /* set the initial position (must be done, before show is called!) */
    gtk_window_set_position(GTK_WINDOW(win), GTK_WIN_POS_CENTER);

    return win;
}


/* Present the created window on the screen. */
void
window_present(GtkWidget *win)
{
    window_geometry_t  geom;
    const gchar       *name;

    /* present this window */
    gtk_window_present(GTK_WINDOW(win));

    /* do we have a previously saved size and position of this window? */
    name = (const gchar *)g_object_get_data(G_OBJECT(win), WINDOW_GEOM_KEY);
    if(name) {
        if(window_geom_load(name, &geom)) {
            /* XXX - use prefs to select which values to set? */
            geom.set_pos        = TRUE;
            geom.set_size       = TRUE;
            geom.set_maximized  = TRUE;
            window_set_geometry(win, &geom);
        }
    }
}


static gboolean
window_key_press_cb(GtkWidget   *widget,
                    GdkEventKey *event,
                    gpointer     cancel_button)
{
    g_return_val_if_fail(widget != NULL, FALSE);
    g_return_val_if_fail(event != NULL, FALSE);

    if (event->keyval == GDK_Escape) {
        gtk_widget_activate(GTK_WIDGET(cancel_button));
        return TRUE;
    }

    return FALSE;
}


/* Set the "key_press_event" signal for a top-level dialog window to
   call a routine to activate the "Cancel" button for a dialog box if
   the key being pressed is the <Esc> key.

   XXX - there should be a GTK+ widget that'll do that for you, and
   let you specify a "Cancel" button.  It should also not impose
   a requirement that there be a separator in the dialog box, as
   the GtkDialog widget does; the visual convention that there's
   such a separator between the rest of the dialog boxes and buttons
   such as "OK" and "Cancel" is, for better or worse, not universal
   (not even in GTK+ - look at the GtkFileSelection dialog!). */
static void
window_set_cancel(GtkWidget *widget,
                  GtkWidget *cancel_button)
{
    g_signal_connect(widget, "key_press_event", G_CALLBACK(window_key_press_cb), cancel_button);
}


/* set the actions needed for the cancel "Close"/"Ok"/"Cancel" button that closes the window */
void
window_set_cancel_button(GtkWidget                *win,
                         GtkWidget                *bt,
                         window_cancel_button_fct  cb)
{
    if(cb)
        g_signal_connect(bt, "clicked", G_CALLBACK(cb), win);

    gtk_widget_grab_default(bt);

    window_set_cancel(win, bt);
}


/* default callback handler for cancel button "clicked" signal */
void
window_cancel_button_cb(GtkWidget *w _U_,
                        gpointer   data)
{
    window_destroy(GTK_WIDGET(data));
}


/* default callback handler: the window managers X of the window was clicked (delete_event) */
gboolean
window_delete_event_cb(GtkWidget *win,
                       GdkEvent  *event _U_,
                       gpointer   user_data _U_)
{
    window_destroy(win);

    /* event handled, don't do anything else */
    return TRUE;
}


/* get the geometry of a window from window_new() */
void
window_get_geometry(GtkWidget         *widget,
                    window_geometry_t *geom)
{
    GdkWindowState state;
    GdkWindow *widget_window;

    /* Try to grab our geometry.

       GTK+ provides two routines to get a window's position relative
       to the X root window.  If I understand the documentation correctly,
       gdk_window_get_deskrelative_origin applies mainly to Enlightenment
       and gdk_window_get_root_origin applies for all other WMs.

       The code below tries both routines, and picks the one that returns
       the upper-left-most coordinates.

       More info at:

        http://mail.gnome.org/archives/gtk-devel-list/2001-March/msg00289.html
        http://www.gtk.org/faq/#AEN606

        As gdk_window_get_deskrelative_origin() is deprecated it has been removed 2011-07-24.
     */

    memset(geom, 0, sizeof(window_geometry_t));

    widget_window = gtk_widget_get_window(widget);

    gdk_window_get_root_origin(widget_window,
                               &geom->x,
                               &geom->y);

    /* XXX - Is this the "approved" method? */
#if GTK_CHECK_VERSION(2,24,0)
    geom->width = gdk_window_get_width(widget_window);
    geom->height = gdk_window_get_height(widget_window);
#else
    ws_gdk_drawable_get_size(widget_window,
                          &geom->width,
                          &geom->height);
#endif
    state = gdk_window_get_state(widget_window);
    geom->maximized = ((state & GDK_WINDOW_STATE_MAXIMIZED) != 0);
}


#ifdef _WIN32
/* Ensure Wireshark isn't obscured by the system taskbar (or other desktop toolbars).
 * Resolves https://bugs.wireshark.org/bugzilla/show_bug.cgi?id=3034 */
static void
window_adjust_if_obscured(window_geometry_t *geom)
{
    MONITORINFO MonitorInfo;
    HMONITOR hMonitor;
    POINT pt, vs;
    DWORD dwFlags = MONITOR_DEFAULTTONEAREST; /* MONITOR_DEFAULTTOPRIMARY? */

    /*
     * Get the virtual screen's top-left coordinates so we can reliably
     * determine which monitor we're dealing with.  See also:
     * http://msdn.microsoft.com/en-us/library/windows/desktop/dd145136%28v=vs.85%29.aspx
     */
    vs.x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    vs.y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    pt.x = geom->x + vs.x;
    pt.y = geom->y + vs.y;
    MonitorInfo.cbSize = sizeof(MONITORINFO);
    hMonitor = MonitorFromPoint(pt, dwFlags);
    if (GetMonitorInfo(hMonitor, &MonitorInfo)) {
        if (pt.x < MonitorInfo.rcWork.left)
            geom->x += MonitorInfo.rcWork.left - pt.x;
        if (pt.y < MonitorInfo.rcWork.top)
            geom->y += MonitorInfo.rcWork.top - pt.y;
    }
}
#endif

/* detect broken GNOME shell fullscreen described in #11303 */
static gboolean
broken_gnome_fullscreen(void) {
    return (0 == g_strcmp0(g_getenv("XDG_CURRENT_DESKTOP"), "GNOME") &&
            (0 == g_strcmp0(g_getenv("GDMSESSION"), "gnome-shell") ||
             0 == g_strcmp0(g_getenv("GDMSESSION"), "gnome-classic") ||
             0 == g_strcmp0(g_getenv("GDMSESSION"), "gnome")));
}

/* set the geometry of a window from window_new() */
void
window_set_geometry(GtkWidget         *widget,
                    window_geometry_t *geom)
{
    GdkScreen *default_screen;
    GdkRectangle viewable_area;
    gint monitor_num;

    /* vertical offset to be set for fixing #11303, it makes room for window */
    /*  titlebar */
    const gint y_offset = (geom->set_maximized &&
                           geom->maximized &&
                           broken_gnome_fullscreen())?WINDOW_DECORATION_SIZE:0;

    /* as we now have the geometry from the recent file, set it */
    /* if the window was minimized, x and y are -32000 (at least on Win32) */
    if (geom->set_pos && geom->x != -32000 && geom->y != -32000) {
        /* Per Wireshark bug #553, GTK has a problem on MS Windows
         * where the upper-left corner of the window may appear off
         * screen when when a single desktop spans multiple monitors
         * of different resolutions and positions relative to each
         * other.
         *
         * If the requested (x,y) position isn't within the monitor's
         * viewable area, change it to the viewable area's (x,y). */
        default_screen = gdk_screen_get_default();
        monitor_num = gdk_screen_get_monitor_at_point(default_screen,
                                                      geom->x, geom->y);
        gdk_screen_get_monitor_geometry(default_screen, monitor_num,
                                        &viewable_area);
        if(geom->x < viewable_area.x || geom->x > (viewable_area.x + viewable_area.width))
            geom->x = viewable_area.x;

        if(geom->y < viewable_area.y || geom->y > (viewable_area.y + viewable_area.height))
            geom->y = viewable_area.y;

        #ifdef _WIN32
        window_adjust_if_obscured(geom);
        #endif

        gtk_window_move(GTK_WINDOW(widget),
                        geom->x,
                        geom->y + y_offset);
    }

    if (geom->set_size) {
        gtk_window_resize(GTK_WINDOW(widget),
        /*gtk_widget_set_size_request(widget,*/
                                geom->width,
                                geom->height - y_offset);
    }

    if(geom->set_maximized) {
        if (geom->maximized) {
            gdk_window_maximize(gtk_widget_get_window(widget));
        } else {
            gdk_window_unmaximize(gtk_widget_get_window(widget));
        }
    }
}

void
window_destroy(GtkWidget *win)
{
    window_geometry_t  geom;
    const gchar       *name;

    if (!win)
        return;

    /* get_geometry must be done *before* destroy is running, as the window geometry
     * cannot be retrieved at destroy time (so don't use event "destroy" for this) */
    /* ...and don't do this at all, if we currently have no GdkWindow (e.g. if the
     * GtkWidget is hidden) */
    if(gtk_widget_get_has_window(win) && gtk_widget_get_visible(win)) {
        window_get_geometry(win, &geom);

        name = (const gchar *)g_object_get_data(G_OBJECT(win), WINDOW_GEOM_KEY);
        if(name) {
            window_geom_save(name, &geom);
            g_free((gpointer)name);
        }
    }

    gtk_widget_destroy(win);
}

static GtkWidget *
_gtk_image_new_from_pixbuf_unref(GdkPixbuf *pixbuf) {
    GtkWidget *widget;

    widget = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    return widget;
}

/* convert an xpm to a GtkWidget */
GtkWidget *
xpm_to_widget(const char **xpm) {
    GdkPixbuf *pixbuf;

    pixbuf = gdk_pixbuf_new_from_xpm_data(xpm);
    return _gtk_image_new_from_pixbuf_unref(pixbuf);
}

/* Convert an pixbuf GResource to a GtkWidget */
GtkWidget *
#ifdef HAVE_GDK_GRESOURCE
pixbuf_to_widget(const char *pb_path) {
#else
pixbuf_to_widget(const guint8 *pb_data) {
#endif
    GdkPixbuf *pixbuf;

#ifdef HAVE_GDK_GRESOURCE
    pixbuf = ws_gdk_pixbuf_new_from_resource(pb_path);
#else
    pixbuf = gdk_pixbuf_new_from_inline(-1, pb_data, FALSE, NULL);
#endif
    return _gtk_image_new_from_pixbuf_unref(pixbuf);
}

/*
 * Alert box for an invalid display filter expression.
 * Assumes "dfilter_error_msg" has been set by "dfilter_compile()" to the
 * error message for the filter.
 *
 * XXX - should this have a "Help" button that pops up the display filter
 * help?
 */
void
bad_dfilter_alert_box(GtkWidget  *parent,
                      const char *dftext,
                      gchar *err_msg)
{
    GtkWidget *msg_dialog;

    msg_dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_MESSAGE_ERROR,
                                        GTK_BUTTONS_OK,
            "The filter expression \"%s\" isn't a valid display filter. (%s)",
                                        dftext, err_msg);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(msg_dialog),
         "See the help for a description of the display filter syntax.");
    gtk_dialog_run(GTK_DIALOG(msg_dialog));
    gtk_widget_destroy(msg_dialog);
}

/* update the main window */
void
main_window_update(void)
{
    while (gtk_events_pending())
        gtk_main_iteration();
}

#ifdef HAVE_LIBPCAP

/* quit a nested main window */
void
main_window_nested_quit(void)
{
    if (gtk_main_level() > 0)
        gtk_main_quit();
}

/* quit the main window */
void
main_window_quit(void)
{
    gtk_main_quit();
}

typedef struct pipe_input_tag {
    gint             source;
    gpointer         user_data;
    ws_process_id   *child_process;
    pipe_input_cb_t  input_cb;
    guint            pipe_input_id;
#ifdef _WIN32
#else
    GIOChannel      *channel;
#endif
} pipe_input_t;


#ifdef _WIN32
/* The timer has expired, see if there's stuff to read from the pipe,
   if so, do the callback */
static gboolean
pipe_timer_cb(gpointer data)
{
    HANDLE        handle;
    DWORD         avail      = 0;
    gboolean      result;
    DWORD         childstatus;
    pipe_input_t *pipe_input = data;
    gint          iterations = 0;


    /* try to read data from the pipe only 5 times, to avoid blocking */
    while(iterations < 5) {
        /*g_log(NULL, G_LOG_LEVEL_DEBUG, "pipe_timer_cb: new iteration");*/

        /* Oddly enough although Named pipes don't work on win9x,
           PeekNamedPipe does !!! */
        handle = (HANDLE)_get_osfhandle(pipe_input->source);
        result = PeekNamedPipe(handle, NULL, 0, NULL, &avail, NULL);

        /* Get the child process exit status */
        GetExitCodeProcess((HANDLE)*(pipe_input->child_process), &childstatus);

        /* If the Peek returned an error, or there are bytes to be read
           or the childwatcher thread has terminated then call the normal
           callback */
        if (!result || avail > 0 || childstatus != STILL_ACTIVE) {

            /*g_log(NULL, G_LOG_LEVEL_DEBUG, "pipe_timer_cb: data avail");*/

            if(pipe_input->pipe_input_id != 0) {
                /*g_log(NULL, G_LOG_LEVEL_DEBUG, "pipe_timer_cb: stop timer");*/
                /* avoid reentrancy problems and stack overflow */
                g_source_remove(pipe_input->pipe_input_id);
                pipe_input->pipe_input_id = 0;
            }

            /* And call the real handler */
            if (!pipe_input->input_cb(pipe_input->source, pipe_input->user_data)) {
                g_log(NULL, G_LOG_LEVEL_DEBUG, "pipe_timer_cb: input pipe closed, iterations: %u", iterations);
                /* pipe closed, return false so that the old timer is not run again */
                return FALSE;
            }
        }
        else {
            /*g_log(NULL, G_LOG_LEVEL_DEBUG, "pipe_timer_cb: no data avail");*/
            /* No data, stop now */
            break;
        }

        iterations++;
    }

    if(pipe_input->pipe_input_id == 0) {
        /* restore pipe handler */
        pipe_input->pipe_input_id = g_timeout_add(200, pipe_timer_cb, data);
        /*g_log(NULL, G_LOG_LEVEL_DEBUG, "pipe_timer_cb: finished with iterations: %u, new timer", iterations);*/

        /* Return false so that the old timer is not run again */
        return FALSE;
    } else {
        /*g_log(NULL, G_LOG_LEVEL_DEBUG, "pipe_timer_cb: finished with iterations: %u, old timer", iterations);*/

        /* we didn't stopped the old timer, so let it run */
        return TRUE;
    }
}

#else /* _WIN32 */

/* There's stuff to read from the sync pipe, meaning the child has sent
   us a message, or the sync pipe has closed, meaning the child has
   closed it (perhaps because it exited). */
static gboolean
pipe_input_cb(GIOChannel   *source _U_,
              GIOCondition  condition _U_,
              gpointer      data)
{
    pipe_input_t *pipe_input = (pipe_input_t *)data;


    /* avoid reentrancy problems and stack overflow */
    g_source_remove(pipe_input->pipe_input_id);

    if (pipe_input->input_cb(pipe_input->source, pipe_input->user_data)) {
        /* restore pipe handler */
        pipe_input->pipe_input_id = g_io_add_watch_full(pipe_input->channel,
                                                        G_PRIORITY_HIGH,
                                                        (GIOCondition)(G_IO_IN|G_IO_ERR|G_IO_HUP),
                                                        pipe_input_cb,
                                                        pipe_input,
                                                        NULL);
    }
    return TRUE;
}
#endif

void
pipe_input_set_handler(gint             source,
                       gpointer         user_data,
                       ws_process_id   *child_process,
                       pipe_input_cb_t  input_cb)
{
    static pipe_input_t pipe_input;

    pipe_input.source        = source;
    pipe_input.child_process = child_process;
    pipe_input.user_data     = user_data;
    pipe_input.input_cb      = input_cb;

#ifdef _WIN32
    /* Tricky to use pipes in win9x, as no concept of wait.  NT can
       do this but that doesn't cover all win32 platforms.  GTK can do
       this but doesn't seem to work over processes.  Attempt to do
       something similar here, start a timer and check for data on every
       timeout. */
       /*g_log(NULL, G_LOG_LEVEL_DEBUG, "pipe_input_set_handler: new");*/
    pipe_input.pipe_input_id = g_timeout_add(200, pipe_timer_cb, &pipe_input);
#else
    pipe_input.channel = g_io_channel_unix_new(source);
    g_io_channel_set_encoding(pipe_input.channel, NULL, NULL);
    pipe_input.pipe_input_id = g_io_add_watch_full(pipe_input.channel,
                                                   G_PRIORITY_HIGH,
                                                   (GIOCondition)(G_IO_IN|G_IO_ERR|G_IO_HUP),
                                                   pipe_input_cb,
                                                   &pipe_input,
                                                   NULL);
#endif
}


#endif /* HAVE_LIBPCAP */

/* Given a pointer to a GtkWidget for a top-level window, raise it and
   de-iconify it.  This routine is used if the user has done something to
   ask that a window of a certain type be popped up when there can be only
   one such window and such a window has already been popped up - we
   pop up the existing one rather than creating a new one.

   XXX - we should request that it be given the input focus, too.  Alas,
   GDK has nothing to do that, e.g. by calling "XSetInputFocus()" in a
   window in X.  Besides, using "XSetInputFocus()" doesn't work anyway,
   apparently due to the way GTK+/GDK manages the input focus.

   The X Desktop Group's Window Manager Standard specifies, in the section
   on Root Window Properties, an _NET_ACTIVE_WINDOW client message that
   can be sent to the root window, containing the window ID of the
   window to activate; I infer that this might be the way to give the
   window the input focus - I assume that means it's also de-iconified,
   but I wouldn't assume it'd raise it.

   XXX - will this do the right thing on window systems other than X? */
void
reactivate_window(GtkWidget *win)
{
    GdkWindow *win_window;

    win_window = gtk_widget_get_window(win);

    gdk_window_show(win_window);
    gdk_window_raise(win_window);
}

/* List of all GtkScrolledWindows, so we can globally set the scrollbar
   placement of all of them. */
static GList *scrolled_windows;

static void setup_scrolled_window(GtkWidget *scrollw);
static void forget_scrolled_window(GtkWidget *scrollw, gpointer data);

/* Create a GtkScrolledWindow, set its scrollbar placement appropriately,
   and remember it. */
GtkWidget *
scrolled_window_new(GtkAdjustment *hadjustment,
                    GtkAdjustment *vadjustment)
{
    GtkWidget *scrollw;

    scrollw = gtk_scrolled_window_new(hadjustment, vadjustment);
    setup_scrolled_window(scrollw);
    return scrollw;
}

/* Set a GtkScrolledWindow's scrollbar placement and add it to the list
   of GtkScrolledWindows. */
static void
setup_scrolled_window(GtkWidget *scrollw)
{
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollw),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(3,16,0)
        gtk_scrolled_window_set_overlay_scrolling(GTK_SCROLLED_WINDOW(scrollw), FALSE);
#endif /* OVERLAY_SCROLLING */

    scrolled_windows = g_list_append(scrolled_windows, scrollw);

    /* Catch the "destroy" event on the widget, so that we remove it from
       the list when it's destroyed. */
    g_signal_connect(scrollw, "destroy", G_CALLBACK(forget_scrolled_window), NULL);
}

/* Remove a GtkScrolledWindow from the list of GtkScrolledWindows. */
static void
forget_scrolled_window(GtkWidget *scrollw,
                       gpointer   data _U_)
{
    scrolled_windows = g_list_remove(scrolled_windows, scrollw);
}

/* List of all CTrees/TreeViews, so we can globally set the line and
 * expander style of all of them. */
static GList *trees;

static void setup_tree(GtkWidget *tree);
static void forget_tree(GtkWidget *tree, gpointer data);
static void set_tree_styles(GtkWidget *tree);
static gboolean tree_view_key_pressed_cb(GtkWidget *tree, GdkEventKey *event, gpointer user_data _U_);

/* Create a Tree, give it the right styles, and remember it. */
GtkWidget *
tree_view_new(GtkTreeModel *model)
{
    GtkWidget *tree;

    tree = gtk_tree_view_new_with_model(model);
    setup_tree(tree);
    return tree;
}

/* Set a Tree's styles and add it to the list of Trees. */
static void
setup_tree(GtkWidget *tree)
{
    set_tree_styles(tree);

    trees = g_list_append(trees, tree);

    /* Catch the "destroy" event on the widget, so that we remove it from
       the list when it's destroyed. */
    g_signal_connect(tree, "destroy", G_CALLBACK(forget_tree), NULL);
    g_signal_connect(tree, "key-press-event", G_CALLBACK(tree_view_key_pressed_cb), NULL );
}

/* Remove a Tree from the list of Trees. */
static void
forget_tree(GtkWidget *tree,
            gpointer   data _U_)
{
    trees = g_list_remove(trees, tree);
}

/* Set the styles of a Tree based upon user preferences. */
static void
set_tree_styles(GtkWidget *tree)
{
    g_assert(prefs.gui_altern_colors >= 0 && prefs.gui_altern_colors <= 1);
    gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree),
                                 prefs.gui_altern_colors);
}

static void
set_tree_styles_cb(gpointer data,
                   gpointer user_data _U_)
{
    set_tree_styles((GtkWidget *)data);
}

/* Set the styles of all Trees based upon style values. */
void
set_tree_styles_all(void)
{
    g_list_foreach(trees, set_tree_styles_cb, NULL);
}

/* Move the currently-selected item in a list store up or down one position. */
gboolean
tree_view_list_store_move_selection(GtkTreeView *tree,
                                    gboolean     move_up)
{
    GtkTreeIter       from, to;
    GtkTreeModel     *model;
    GtkTreeSelection *sel;
    GtkTreePath      *path_from, *path_to;

    sel = gtk_tree_view_get_selection(tree);
    if (!gtk_tree_selection_get_selected(sel, &model, &from)) {
        return FALSE;
    }

    path_from = gtk_tree_model_get_path(model, &from);
    if (!path_from) {
        return FALSE;
    }

    path_to = gtk_tree_path_copy(path_from);
    /* XXX - Why does one return void and the other return a gboolean? */
    if (move_up) {
        gtk_tree_path_prev(path_to);
    } else {
        gtk_tree_path_next(path_to);
    }

    if (gtk_tree_path_compare(path_from, path_to) == 0) {
        gtk_tree_path_free(path_from);
        gtk_tree_path_free(path_to);
        return FALSE;
    }

    gtk_tree_model_get_iter(model, &to, path_to);
    gtk_list_store_swap(GTK_LIST_STORE(model), &from, &to);
    gtk_tree_path_free(path_from);
    gtk_tree_path_free(path_to);
    return TRUE;
}

/* Find the selected row number in a list store. */
gint
tree_view_list_store_get_selected_row(GtkTreeView *tree) {
    GtkTreeIter       iter;
    GtkTreeModel     *model;
    GtkTreeSelection *sel;
    GtkTreePath      *path;
    gchar            *path_str;
    gint              row;

    sel = gtk_tree_view_get_selection(tree);
    if (!gtk_tree_selection_get_selected(sel, &model, &iter)) {
        return -1;
    }

    path = gtk_tree_model_get_path(model, &iter);
    if (!path) {
        return FALSE;
    }

    path_str = gtk_tree_path_to_string(path);
    gtk_tree_path_free(path);

    row = (gint)strtol(path_str, NULL, 10);
    g_free(path_str);

    return row;
}

/* append a row to the simple list */
/* use it like: simple_list_append(list, 0, "first", 1, "second", -1) */
void
simple_list_append(GtkWidget *list,
                   ...)
{
    va_list ap;

    GtkTreeIter   iter;
    GtkListStore *store;

    va_start(ap, list);
    store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(list)));
    gtk_list_store_append(store, &iter);
    gtk_list_store_set_valist(store, &iter, ap);
    va_end(ap);
}

/* create a simple list widget */
GtkWidget *
simple_list_new(gint   cols,
                const gchar **titles) {
    GtkWidget         *plugins_list;
    int                i;
    GtkListStore      *store;
    GtkCellRenderer   *renderer;
    GtkTreeViewColumn *column;


    g_assert(cols <= 10);
    store = gtk_list_store_new(cols,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    plugins_list = tree_view_new(GTK_TREE_MODEL(store));
    g_object_unref(G_OBJECT(store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(plugins_list), (titles != NULL));
    for(i=0; i<cols; i++) {
        renderer = gtk_cell_renderer_text_new();
        column = gtk_tree_view_column_new_with_attributes(titles ? titles[i] : "", renderer,
                                                          "text", i, NULL);
        gtk_tree_view_column_set_sort_column_id(column, i);
        gtk_tree_view_append_column(GTK_TREE_VIEW(plugins_list), column);
    }

    return plugins_list;
}

void
render_as_url(GtkCellRenderer *cell)
{
    g_object_set(cell, "foreground", "blue", NULL);
    g_object_set(cell, "foreground-set", TRUE, NULL);

    g_object_set(cell, "underline", PANGO_UNDERLINE_SINGLE, NULL);
    g_object_set(cell, "underline-set", TRUE, NULL);
}

void
simple_list_url_col(GtkWidget *list,
                    gint       col)
{
    GtkTreeViewColumn *ul_column;
    GList             *renderers_list;
    GtkCellRenderer   *ul_renderer;

    /* make the column look like a link ... */
    ul_column = gtk_tree_view_get_column(GTK_TREE_VIEW(list), col);

    renderers_list = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(ul_column));

    if(renderers_list != NULL) {
        /* it is simple list - there should be only one renderer */
        ul_renderer = (GtkCellRenderer*)renderers_list->data;

        render_as_url(ul_renderer);

        g_list_free(renderers_list);
    }

}

void
copy_to_clipboard(GString *str)
{
    GtkClipboard *cb;

    cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);     /* Get the default clipboard */
    gtk_clipboard_set_text(cb, str->str, -1);            /* Copy the byte data into the clipboard */
}


typedef struct _copy_binary_t {
    guint8 *data;
    int     len;
} copy_binary_t;

static copy_binary_t *
create_copy_binary_t(const guint8 *data,
                     int           len)
{
    copy_binary_t* copy_data;

    g_assert(len > 0);
    copy_data = g_new(copy_binary_t,1);
    copy_data->data = g_new(guint8,len);
    copy_data->len = len;
    memcpy(copy_data->data,data,len * sizeof(guint8));
    return copy_data;
}

static void
destroy_copy_binary_t(copy_binary_t *copy_data) {
    g_free(copy_data->data);
    g_free(copy_data);
}

static void
copy_binary_free_cb(GtkClipboard  *clipboard _U_,
                         gpointer  user_data_or_owner)
{
    destroy_copy_binary_t((copy_binary_t*)user_data_or_owner);
}

static void
copy_binary_get_cb(GtkClipboard     *clipboard _U_,
                   GtkSelectionData *selection_data,
                   guint             info _U_,
                   gpointer          user_data_or_owner)
{
    copy_binary_t* copy_data;

    copy_data = (copy_binary_t*)user_data_or_owner;

    /* Just do a dumb set as binary data */
    gtk_selection_data_set(selection_data, GDK_NONE, 8, copy_data->data, copy_data->len);
}

void
copy_binary_to_clipboard(const guint8 *data_p,
                         int           len)
{
    static GtkTargetEntry target_entry[] = {
        {"application/octet-stream", 0, 0}};
    /* XXX - this is not understood by most applications,
     * but can be pasted into the better hex editors - is
     * there something better that we can do?
     */

    GtkClipboard  *cb;
    copy_binary_t *copy_data;
    gboolean       ret;

    if(len <= 0) {
        return; /* XXX would it be better to clear the clipboard? */
    }
    cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);     /* Get the default clipboard */
    copy_data = create_copy_binary_t(data_p,len);

    ret = gtk_clipboard_set_with_data(cb,target_entry,1,
        copy_binary_get_cb, copy_binary_free_cb,copy_data);

    if(!ret) {
        destroy_copy_binary_t(copy_data);
    }
}

/*
 * Create a new window title string with user-defined title preference.
 * (Or ignore it if unspecified).
 */
gchar *
create_user_window_title(const gchar *caption)
{
    /* fail-safe */
    if (caption == NULL)
        return g_strdup("");

    /* no user-defined title specified */
    if ((prefs.gui_window_title == NULL) || (*prefs.gui_window_title == '\0'))
        return g_strdup(caption);

    return g_strdup_printf("%s   [%s]", caption, prefs.gui_window_title);
}

/*
 * Set the title of a window based on a supplied caption and the
 * display name for the capture file.
 *
 * XXX - should this include the user preference as well?
 */
void
set_window_title(GtkWidget   *win,
                 const gchar *caption)
{
    char *title;
    char *display_name;

    display_name = cf_get_display_name(&cfile);
    title = g_strdup_printf("%s: %s", caption, display_name);
    g_free(display_name);
    gtk_window_set_title(GTK_WINDOW(win), title);
    g_free(title);
}

/*
 * Collapse row and his children
 */
static void
tree_collapse_row_with_children(GtkTreeView *tree_view, GtkTreeModel *model, GtkTreePath *path,
                   GtkTreeIter *iter)
{
    GtkTreeIter child;

    if (gtk_tree_view_row_expanded(tree_view, path)) {
        if (gtk_tree_model_iter_children(model, &child, iter)) {
            gtk_tree_path_down(path);

        do {

                if (gtk_tree_view_row_expanded(tree_view, path)) {
                    tree_collapse_row_with_children(tree_view, model, path, &child);
                }

                gtk_tree_path_next(path);
            } while (gtk_tree_model_iter_next(model, &child));

            gtk_tree_path_up(path);

            gtk_tree_view_collapse_row(tree_view, path);
        }
    }
}

void
tree_collapse_path_all(GtkTreeView *tree_view, GtkTreePath *path)
{
    GtkTreeIter iter;
    GtkTreeModel *model;

    model = gtk_tree_view_get_model(tree_view);
    gtk_tree_model_get_iter(model, &iter, path);

    tree_collapse_row_with_children(tree_view, model, path, &iter);
}

/*
 * This callback is invoked when keyboard focus is within either
 * the packetlist view or the detail view.  The keystrokes processed
 * within this callback are attempting to modify the detail view.
 * Within the detail view we special case the Left Arrow, Backspace
 * and Enter keys depending on the state of the expander (if any)
 * for the item in focus.
 *
 * Returning FALSE allows processing of the original key_press_event
 * by other callbacks.  Left/Right scrolling of the packetlist
 * view and expanding/collapsing of the detail view lists is
 * handled by the default GtkTreeView key-press-event call back.
 *
 * XXX - Would an improved version of this callback test to see which
 * of the two GtkTreeView lists has focus?  Left/Right scrolling of
 * the packetlist is currently not optimal.  It will take several
 * right or left keypress events before the packetlist responds.
 * The problem appears to be that the focus is on a particular cell
 * within the highlighted row cell (like a spreadsheet).  Scrolling
 * of the view  right or left will not occur until the focus is
 * moved to a cell off the left or right edge of the packet list
 * view.  Also TAB/SHIFT-TAB events can move keyboard focus to
 * the packetlist header where there is currently visual hint
 * a header cell has focus.
 */
static gboolean
tree_view_key_pressed_cb(GtkWidget   *tree,
                         GdkEventKey *event,
                         gpointer     user_data _U_)
{
    GtkTreeSelection *selection;
    GtkTreeIter iter;
    GtkTreeIter parent;
    GtkTreeModel *model;
    GtkTreePath *path;
    gboolean    expanded, expandable;
    int rc = FALSE;

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    if(!selection) {
        return FALSE;
    }

    if(!gtk_tree_selection_get_selected(selection, &model, &iter)) {
        return FALSE;
    }

    path = gtk_tree_model_get_path(model, &iter);
    if(!path) {
        return FALSE;
    }

    /* Always FALSE when we're in the packet list (at least until we add sub-packets) */
    expanded = gtk_tree_view_row_expanded(GTK_TREE_VIEW(tree), path);
    expandable = gtk_tree_model_iter_has_child(model, &iter);

    switch (event->keyval) {
        case GDK_Left:
            if(expanded) {
                /* Subtree is expanded. Collapse it. */
                if (event->state & GDK_SHIFT_MASK)
                {
                    tree_collapse_row_with_children(GTK_TREE_VIEW(tree), model, path, &iter);
                }
                else
                    gtk_tree_view_collapse_row(GTK_TREE_VIEW(tree), path);
                rc = TRUE;
                break;
            }
            /* No break - fall through to jumping to the parent */
        case GDK_BackSpace:
            if (!expanded) {
                /* subtree is already collapsed, jump to parent node */
                if(! gtk_tree_model_iter_parent(model, &parent, &iter)) {
                    rc = FALSE;
                    break;
                }
                gtk_tree_path_free(path);
                path = gtk_tree_model_get_path(model, &parent);
                if(!path) {
                    return FALSE;
                }
                gtk_tree_view_set_cursor(GTK_TREE_VIEW(tree), path,
                                                 NULL /* focus_column */,
                                                 FALSE /* !start_editing */);
                rc = TRUE;
                break;
            }
            break;
        case GDK_Right:
            if (expandable) {
                /* We have a subtree. Try to expand it. */
                gtk_tree_view_expand_row(GTK_TREE_VIEW(tree), path, FALSE /* !open_all */);
                rc = TRUE;
                break;
            } else {
                rc = FALSE;
                break;
            }
        case GDK_Return:
        case GDK_KP_Enter:
            /* Reverse the current state. */
            if (expanded)
                gtk_tree_view_collapse_row(GTK_TREE_VIEW(tree), path);
            else
                gtk_tree_view_expand_row(GTK_TREE_VIEW(tree), path, FALSE /* !open_all */);
            rc = TRUE;
            break;
    }

    if(path) {
        gtk_tree_path_free(path);
    }
    return rc;
}

void
switch_to_fixed_col(GtkTreeView *view)
{
    gint size;
    GtkTreeViewColumn *column;
    GList             *columns, *list;

    columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(view));
    list = columns;
    while(columns) {
        column = (GtkTreeViewColumn *)columns->data;
        size = gtk_tree_view_column_get_width(column);
        gtk_tree_view_column_set_sizing(column,GTK_TREE_VIEW_COLUMN_FIXED);
        if (size > gtk_tree_view_column_get_fixed_width(column))
            gtk_tree_view_column_set_fixed_width(column, size);
        columns = g_list_next(columns);
    }
    g_list_free(list);

    gtk_tree_view_set_fixed_height_mode(view, TRUE);
}

gint
get_default_col_size(GtkWidget   *view,
                     const gchar *str)
{
    PangoLayout *layout;
    gint col_width;

    layout = gtk_widget_create_pango_layout(view, str);
    pango_layout_get_pixel_size(layout,
                                &col_width, /* width */
                                NULL); /* height */
    g_object_unref(G_OBJECT(layout));
    /* Add a single character's width to get some spacing between columns */
    return col_width + (pango_font_description_get_size(user_font_get_regular()) / PANGO_SCALE);
}


/*
 * This function can be called from gtk_tree_view_column_set_cell_data_func()
 * the user data must be the column number.
 * Present floats with two decimals
 */
void
float_data_func(GtkTreeViewColumn *column _U_,
                GtkCellRenderer   *renderer,
                GtkTreeModel      *model,
                GtkTreeIter       *iter,
                gpointer           user_data)
{
    gfloat  float_val;
    gchar   buf[20];
    char *savelocale;

    /* the col to get data from is in userdata */
    gint float_col = GPOINTER_TO_INT(user_data);

    gtk_tree_model_get(model, iter, float_col, &float_val, -1);

    /* save the current locale */
    savelocale = g_strdup(setlocale(LC_NUMERIC, NULL));
    /* switch to "C" locale to avoid problems with localized decimal separators
     * in g_snprintf("%f") functions
     */
    setlocale(LC_NUMERIC, "C");

    g_snprintf(buf, sizeof(buf), "%.2f", float_val);
    /* restore previous locale setting */
    setlocale(LC_NUMERIC, savelocale);
    g_free(savelocale);

    g_object_set(renderer, "text", buf, NULL);
}

/*
 * This function can be called from gtk_tree_view_column_set_cell_data_func()
 * the user data must be the column number.
 * Present value as hexadecimal.
 */
void
present_as_hex_func(GtkTreeViewColumn *column _U_,
                    GtkCellRenderer   *renderer,
                    GtkTreeModel      *model,
                    GtkTreeIter       *iter,
                    gpointer           user_data)
{
    guint  val;
    gchar   buf[35];

    /* the col to get data from is in userdata */
    gint col = GPOINTER_TO_INT(user_data);

    gtk_tree_model_get(model, iter, col, &val, -1);

    g_snprintf(buf, sizeof(buf), "0x%02x", val);

    g_object_set(renderer, "text", buf, NULL);
}

void
u64_data_func(GtkTreeViewColumn *column _U_,
              GtkCellRenderer   *renderer,
              GtkTreeModel      *model,
              GtkTreeIter       *iter,
              gpointer           user_data)
{
    guint64 val;
    int i = 0;
    gchar *bp;
    gchar   buf[35];

    /* the col to get data from is in userdata */
    gint col = GPOINTER_TO_INT(user_data);

    gtk_tree_model_get(model, iter, col, &val, -1);

    bp = &buf[34];
    *bp = 0;
    do {
        *--bp = (gchar)(val % 10) +'0';
        if (!(++i % 3)) {
            *--bp = ' ';
        }
    } while ((val /= 10) != 0 && bp > buf);
    g_object_set(renderer, "text", bp, NULL);
}

/*
 * This function can be called from gtk_tree_view_column_set_cell_data_func()
 * The user data must be the column number.
 * Renders the const static string whose pointer is stored.
 */
void
str_ptr_data_func(GtkTreeViewColumn *column  _U_,
                  GtkCellRenderer   *renderer,
                  GtkTreeModel      *model,
                  GtkTreeIter       *iter,
                  gpointer          user_data)
{
    const gchar *str = NULL;

    /* The col to get data from is in userdata */
    gint data_column = GPOINTER_TO_INT(user_data);

    gtk_tree_model_get(model, iter, data_column, &str, -1);
    /* XXX should we check that str is non NULL and print a warning or do assert? */

    g_object_set(renderer, "text", str, NULL);
}

gint
str_ptr_sort_func(GtkTreeModel *model,
                  GtkTreeIter  *a,
                  GtkTreeIter  *b,
                  gpointer      user_data)
{
    const gchar *str_a = NULL;
    const gchar *str_b = NULL;
    gint ret = 0;

    /* The col to get data from is in userdata */
    gint data_column = GPOINTER_TO_INT(user_data);

    gtk_tree_model_get(model, a, data_column, &str_a, -1);
    gtk_tree_model_get(model, b, data_column, &str_b, -1);

    if (str_a == str_b) {
        /* it's worth testing because a lot of rows point to the same data */
        return 0;
    }
    else if (str_a == NULL || str_b == NULL) {
        ret = (str_a == NULL) ? -1 : 1;
    }
    else {
        ret = g_ascii_strcasecmp(str_a,str_b);
    }
    return ret;
}

/** --------------------------------------------------
 * ws_combo_box_text_and_pointer convenience functions
 *  (Code adapted from GtkComboBox.c)
 */

/**
 * ws_combo_box_new_text_and_pointer_full:
 *
 * Convenience function which constructs a new "text and pointer" combo box, which
 * is a #GtkComboBox just displaying strings and storing a pointer associated with
 * each combo_box entry; The pointer can be retrieved when an entry is selected.
 * Also: optionally returns the cell renderer for the combo box.
 * If you use this function to create a text_and_pointer combo_box,
 * you should only manipulate its data source with the
 * following convenience functions:
 *   ws_combo_box_append_text_and_pointer()
 *   ws_combo_box_append_text_and_pointer_full()
 *
 * @param cell_p  pointer to return the 'GtkCellRenderer *' for the combo box (or NULL).
 * @return A pointer to a new text_and_pointer combo_box.
 */

/* Note:
 * GtkComboBox style property: "appears-as-list":
 *   Default: 0: ie: displays as menus
 *   Wireshark Windows gtkrc: 1: ie: displays as lists (treeview)
 */
GtkWidget *
ws_combo_box_new_text_and_pointer_full(GtkCellRenderer **cell_p) {
    GtkWidget       *combo_box;
    GtkCellRenderer *cell;
    GtkTreeStore    *store;

    /* The Tree store for the GtkComboBox has 3 columns:
       0: text string for display in GtkComboBox list;
       1: pointer (data) associated with the entry;
       2: True/False depending upon whether this entry is selectable ("sensitive" attribute).
    */

    store = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_BOOLEAN);
    combo_box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);
    cell = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo_box), cell, TRUE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo_box), cell,
                                   "text", 0, "sensitive", 2,
                                   NULL);
    if (cell_p != NULL) {
        *cell_p = cell;
    }
    return combo_box;
}

/**
 * ws_combo_box_new_text_and_pointer:
 *
 * Convenience function which constructs a new "text and pointer" combo box, which
 * is a #GtkComboBox just displaying strings and storing a pointer associated with
 * each combo_box entry; The pointer can be retrieved when an entry is selected.
 * If you use this function to create a text_and_pointer combo_box,
 * you should only manipulate its data source with the
 * following convenience functions:
 *   ws_combo_box_append_text_and_pointer()
 *   ws_combo_box_append_text_and_pointer_full()
 *
 * @return A pointer to a new text_and_pointer combo_box.
 */

GtkWidget *
ws_combo_box_new_text_and_pointer(void) {
    return ws_combo_box_new_text_and_pointer_full(NULL);
}


/**
 * ws_combo_box_clear_text_and_pointer:
 * @param combo_box A #GtkComboBox constructed using ws_combo_box_new_text_and_pointer()
 *
 * Clears all the text_and_pointer entries in the text_and_pointer combo_box.
 * Note: A "changed" signal will be emitted after the clear if there was
 * an active (selected) entry before the clear.
 * You should use this function only with combo boxes constructed with
 * ws_combo_box_new_text_and_pointer().
 */
void
ws_combo_box_clear_text_and_pointer(GtkComboBox *combo_box)
{
    gtk_tree_store_clear(GTK_TREE_STORE(gtk_combo_box_get_model(combo_box)));
}

/**
 * ws_combo_box_append_text_and_pointer_full:
 * @param combo_box A #GtkComboBox constructed using ws_combo_box_new_text_and_pointer()
 * @param parent_iter Parent row for apending; NULL if appending to tree top-level;
 * @param text A string to be displayed as an entry in the dropdown list of the combo_box
 * @param ptr  A pointer to be associated with this entry of the combo_box
 * @param sensitive TRUE/FALSE to set sensitivity of the entry
 * @return A GtkTreeIter pointing to the appended GtkVomboBox entry.
 *
 * Appends text and ptr to the list of strings and pointers stored in combo_box.
 * The text and ptr can be appended to any existing level of the tree_store.
 * The sensitivity of the row will be set as requested.
 * Note that you can only use this function with combo boxes constructed with
 * ws_combo_box_new_text_and_pointer().
 */
GtkTreeIter
ws_combo_box_append_text_and_pointer_full(GtkComboBox    *combo_box,
                                          GtkTreeIter    *parent_iter,
                                          const gchar    *text,
                                          gconstpointer   ptr,
                                          gboolean        sensitive)
{
    GtkTreeIter   iter;
    GtkTreeStore *store;

    store = GTK_TREE_STORE(gtk_combo_box_get_model(combo_box));

    gtk_tree_store_append(store, &iter, parent_iter);
    gtk_tree_store_set(store, &iter, 0, text, 1, ptr, 2, sensitive, -1);

    return iter;
}

/**
 * ws_combo_box_append_text_and_pointer:
 * @param combo_box A #GtkComboBox constructed using ws_combo_box_new_text_and_pointer()
 * @param text A string to be displayed as an entry in the dropdown list of the combo_box
 * @param ptr  A pointer to be associated with this entry of the combo_box
 * @return A GtkTreeIter pointing to the appended GtkComboBox entry.
 *
 * Appends text and ptr to the list of strings and pointers stored in combo_box. Note that
 * you can only use this function with combo boxes constructed with
 * ws_combo_box_new_text_and_pointer().
 */
GtkTreeIter
ws_combo_box_append_text_and_pointer(GtkComboBox    *combo_box,
                                     const gchar    *text,
                                     gconstpointer   ptr)
{
    return ws_combo_box_append_text_and_pointer_full(combo_box, NULL, text, ptr, TRUE);
}


/**
 * ws_combo_box_get_active_pointer:
 * @param combo_box A #GtkComboBox constructed using ws_combo_box_new_text_and_pointer()
 * @param ptr  A pointer to a location in which to store the pointer associated with the active entry
 * @return TRUE if an entry is               selected (i.e: an active entry exists); FALSE otherwise
 *
 * You can only use this function with combo boxes constructed with
 * ws_combo_box_new_text_and_pointer().
 */
gboolean
ws_combo_box_get_active_pointer(GtkComboBox *combo_box,
                                gpointer    *ptr)
{
    GtkTreeStore *store;
    GtkTreeIter iter;

    *ptr = NULL;

    if (gtk_combo_box_get_active_iter(combo_box, &iter)) {
        store = GTK_TREE_STORE(gtk_combo_box_get_model(combo_box));
        gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
                           1, ptr, -1);
        return TRUE;
    }
    return FALSE;
}

/**
 * ws_combo_box_get_active:
 * @param combo_box A #GtkComboBox constructed using ws_combo_box_new_text_and_pointer()
 * @return Index of the active entry; -1 if no entry is selected;
 *         Note: If the active item is not an immediate child of root of the tree then
 *          the index returned is that of the top-level for the acftive entry.
 */
gint
ws_combo_box_get_active(GtkComboBox *combo_box)
{
    return gtk_combo_box_get_active(combo_box);
}

/**
 * ws_combo_box_set_active:
 * @param combo_box A #GtkComboBox constructed using ws_combo_box_new_text_and_pointer()
 * @param idx of the entry which is to be set as active (ie: selected).
 *        Index refers to the immediate children of the tree.
 */
void
ws_combo_box_set_active(GtkComboBox *combo_box,
                        gint         idx)
{
    gtk_combo_box_set_active(combo_box, idx);
}

/**
 * ws_combo_box_set_active_iter:
 * @param combo_box A #GtkComboBox constructed using ws_combo_box_new_text_and_pointer()
 * @param iter of the entry which is to be set as active (ie: selected).
 */
void
ws_combo_box_set_active_iter(GtkComboBox *combo_box, GtkTreeIter *iter)
{
    gtk_combo_box_set_active_iter(combo_box, iter);
}


/* Copy functions from GTK 3.0 to be used if GTK version is 2.22 or 2.24 to be able save Graphs to file */
#if GTK_CHECK_VERSION(2,22,0)
#if !GTK_CHECK_VERSION(3,0,0)
static cairo_format_t
gdk_cairo_format_for_content(cairo_content_t content)
{
    switch (content)
    {
    case CAIRO_CONTENT_COLOR:
        return CAIRO_FORMAT_RGB24;
    case CAIRO_CONTENT_ALPHA:
        return CAIRO_FORMAT_A8;
    case CAIRO_CONTENT_COLOR_ALPHA:
    default:
        return CAIRO_FORMAT_ARGB32;
    }
}

static cairo_surface_t *
gdk_cairo_surface_coerce_to_image(cairo_surface_t *surface,
                                  cairo_content_t  content,
                                  int              src_x,
                                  int              src_y,
                                  int              width,
                                  int              height)
{
    cairo_surface_t *copy;
    cairo_t *cr;

    copy = cairo_image_surface_create(gdk_cairo_format_for_content(content),
                                      width,
                                      height);

    cr = cairo_create(copy);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, surface, -src_x, -src_y);
    cairo_paint(cr);
    cairo_destroy(cr);

    return copy;
}

static void
convert_alpha(guchar *dest_data,
              int     dest_stride,
              guchar *src_data,
              int     src_stride,
              int     src_x,
              int     src_y,
              int     width,
              int     height)
{
    int x, y;

    src_data += src_stride * src_y + src_x * 4;

    for (y = 0; y < height; y++) {
        guint32 *src = (guint32 *)src_data;

        for (x = 0; x < width; x++) {
            guint alpha = src[x] >> 24;

            if (alpha == 0)
            {
                dest_data[x * 4 + 0] = 0;
                dest_data[x * 4 + 1] = 0;
                dest_data[x * 4 + 2] = 0;
            }
            else
            {
                dest_data[x * 4 + 0] = (((src[x] & 0xff0000) >> 16) * 255 + alpha / 2) / alpha;
                dest_data[x * 4 + 1] = (((src[x] & 0x00ff00) >>  8) * 255 + alpha / 2) / alpha;
                dest_data[x * 4 + 2] = (((src[x] & 0x0000ff) >>  0) * 255 + alpha / 2) / alpha;
            }
            dest_data[x * 4 + 3] = alpha;
        }

        src_data += src_stride;
        dest_data += dest_stride;
    }
}

static void
convert_no_alpha(guchar *dest_data,
                 int     dest_stride,
                 guchar *src_data,
                 int     src_stride,
                 int     src_x,
                 int     src_y,
                 int     width,
                 int     height)
{
    int x, y;

    src_data += src_stride * src_y + src_x * 4;

    for (y = 0; y < height; y++) {
        guint32 *src = (guint32 *)src_data;

        for (x = 0; x < width; x++) {
            dest_data[x * 3 + 0] = src[x] >> 16;
            dest_data[x * 3 + 1] = src[x] >>  8;
            dest_data[x * 3 + 2] = src[x];
        }

        src_data += src_stride;
        dest_data += dest_stride;
    }
}

/**
 * gdk_pixbuf_get_from_surface:
 * @surface: surface to copy from
 * @src_x: Source X coordinate within @surface
 * @src_y: Source Y coordinate within @surface
 * @width: Width in pixels of region to get
 * @height: Height in pixels of region to get
 *
 * Transfers image data from a #cairo_surface_t and converts it to an RGB(A)
 * representation inside a #GdkPixbuf. This allows you to efficiently read
 * individual pixels from cairo surfaces. For #GdkWindows, use
 * gdk_pixbuf_get_from_window() instead.
 *
 * This function will create an RGB pixbuf with 8 bits per channel.
 * The pixbuf will contain an alpha channel if the @surface contains one.
 *
 * Return value: (transfer full): A newly-created pixbuf with a reference
 *     count of 1, or %NULL on error
 */
GdkPixbuf *
gdk_pixbuf_get_from_surface(cairo_surface_t *surface,
                            gint             src_x,
                            gint             src_y,
                            gint             width,
                            gint             height)
{
    cairo_content_t  content;
    GdkPixbuf       *dest;

    /* General sanity checks */
    g_return_val_if_fail(surface != NULL, NULL);
    g_return_val_if_fail(width > 0 && height > 0, NULL);

    content = (cairo_content_t)(cairo_surface_get_content(surface) | CAIRO_CONTENT_COLOR);
    dest    = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
                             !!(content & CAIRO_CONTENT_ALPHA),
                             8,
                             width, height);

    surface = gdk_cairo_surface_coerce_to_image(surface, content,
                                                src_x, src_y,
                                                width, height);
    cairo_surface_flush(surface);
    if (cairo_surface_status(surface) || dest == NULL)
    {
        cairo_surface_destroy(surface);
        return NULL;
    }

    if (gdk_pixbuf_get_has_alpha(dest))
        convert_alpha(gdk_pixbuf_get_pixels(dest),
                      gdk_pixbuf_get_rowstride(dest),
                      cairo_image_surface_get_data(surface),
                      cairo_image_surface_get_stride(surface),
                      0, 0,
                      width, height);
    else
        convert_no_alpha(gdk_pixbuf_get_pixels(dest),
                         gdk_pixbuf_get_rowstride(dest),
                         cairo_image_surface_get_data(surface),
                         cairo_image_surface_get_stride(surface),
                         0, 0,
                         width, height);

    cairo_surface_destroy(surface);
    return dest;
}
#endif /* !GTK_CHECK_VERSION(3,0,0) */
#endif /* GTK_CHECK_VERSION(2,22,0) */


GtkWidget *
ws_gtk_box_new(GtkOrientation orientation,
               gint           spacing,
               gboolean       homogeneous)
{
#if !GTK_CHECK_VERSION(3,0,0)
    if (orientation == GTK_ORIENTATION_HORIZONTAL)
        return ws_gtk_hbox_new(homogeneous, spacing);
    else
        return ws_gtk_vbox_new(homogeneous, spacing);
#else
    GtkWidget *widget;

    widget = gtk_box_new(orientation, spacing);
    gtk_box_set_homogeneous(GTK_BOX(widget), homogeneous);

    return widget;
#endif /* GTK_CHECK_VERSION(3,0,0) */
}

#if !GTK_CHECK_VERSION(3,0,0)
GtkWidget *
gtk_button_box_new(GtkOrientation orientation)
{
    if (orientation == GTK_ORIENTATION_HORIZONTAL) {
        return ws_gtk_hbutton_box_new();
    } else {
        return gtk_vbutton_box_new();
    }
}

GtkWidget *
gtk_scrollbar_new(GtkOrientation  orientation,
                  GtkAdjustment  *adjustment)
{
    if (orientation == GTK_ORIENTATION_HORIZONTAL) {
        return gtk_hscrollbar_new(adjustment);
    } else {
        return gtk_vscrollbar_new(adjustment);
    }
}

GtkWidget *
gtk_paned_new(GtkOrientation orientation)
{
    if(orientation == GTK_ORIENTATION_HORIZONTAL) {
        return gtk_hpaned_new();
    } else {
        return gtk_vpaned_new();
    }
}

GtkWidget *
gtk_separator_new(GtkOrientation orientation)
{
    if (orientation == GTK_ORIENTATION_HORIZONTAL) {
        return gtk_hseparator_new();
    } else {
        return gtk_vseparator_new();
    }
}
#endif /* GTK_CHECK_VERSION(3,0,0) */

GtkWidget *
frame_new(const gchar *title) {
    GtkWidget *frame, *frame_lb;
    GString *mu_title = g_string_new("");

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    if (title) {
#if defined(_WIN32) || defined(__APPLE__)
        g_string_printf(mu_title, "%s", title);
#else
        g_string_printf(mu_title, "<b>%s</b>", title);
#endif
        frame_lb = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(frame_lb), mu_title->str);
        gtk_frame_set_label_widget(GTK_FRAME(frame), frame_lb);
    }
    g_string_free(mu_title, TRUE);

    return frame;
}


/* ---------------------------------
 * ws_gtk_grid...() wrappers
 * See gui_utils.h
 */

#if !GTK_CHECK_VERSION(3,0,0)
#else  /* GTK3 */

void
ws_gtk_grid_attach_defaults(GtkGrid *grid, GtkWidget *child, gint left, gint top, gint width, gint height)
{
    /* Use defaults for [x|y]options and [x|y]padding which match those for gtk_table_attach_defaults() */
    ws_gtk_grid_attach_extended(grid, child, left, top, width, height,
          (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), (GtkAttachOptions)(GTK_EXPAND|GTK_FILL), 0, 0);
}

void
ws_gtk_grid_attach_extended(GtkGrid *grid, GtkWidget *child,
                            gint left, gint top, gint width, gint height,
                            GtkAttachOptions xoptions, GtkAttachOptions yoptions,
                            guint xpadding, guint ypadding)
{
    gtk_grid_attach(grid, child, left, top, width, height);

    /* XXX: On Gtk3, there's Some trickyness about EXPAND which I probably don't
     *      really understand.
     *      It seems that:
     *       Default for EXPAND is "not set".
     *         In this case "computed expand" based on any child(ren) of this widget will
     *         affect this widget.
     *       If EXPAND is set (either TRUE or FALSE) then the value overrides any effect
     *         from children.
     */

    /* Note: widget defaults are FALSE  */
    if (xoptions & GTK_EXPAND)
        gtk_widget_set_hexpand(child, TRUE);
    if (yoptions & GTK_EXPAND)
        gtk_widget_set_vexpand(child, TRUE);

    /* Note: widget default is GTK_FILL */
    /* XXX: Is an  'align' ignored if the corresponding 'fill; is FALSE ? */
    /* XXX: don't set FILL(since is dedault) but just clear if not set ?? */
    /*      ToDo: review effect of explicit set/clear vs explict clear only */
    gtk_widget_set_halign(child, (xoptions & GTK_FILL) ? GTK_ALIGN_FILL : GTK_ALIGN_CENTER);
    gtk_widget_set_valign(child, (yoptions & GTK_FILL) ? GTK_ALIGN_FILL : GTK_ALIGN_CENTER);

    if (xpadding != 0) {
        gtk_widget_set_margin_left(child, xpadding);
        gtk_widget_set_margin_right(child, xpadding);
    }
    if (ypadding != 0) {
        gtk_widget_set_margin_top(child, ypadding);
        gtk_widget_set_margin_bottom(child, ypadding);
    }
}

void
ws_gtk_grid_set_homogeneous(GtkGrid *grid, gboolean homogeneous)
{
    gtk_grid_set_row_homogeneous(grid, homogeneous);
    gtk_grid_set_column_homogeneous(grid, homogeneous);
}
#endif /* !GTK_CHECK_VERSION(3,0,0) */

/*
 * Wrap gdk_cairo_set_source_color() with the GTK 3 equivalent
 * to be used in GTK2
 */
#if !GTK_CHECK_VERSION(3,0,0)
void
gdk_cairo_set_source_rgba(cairo_t *cr, const GdkRGBA *rgba)
{
    GdkColor color;

    gdkRGBAcolor_to_GdkColor(&color, rgba);

    gdk_cairo_set_source_color(cr, &color);

}
#endif /* GTK_CHECK_VERSION(3,0,0) */

#ifdef HAVE_GDK_GRESOURCE
GdkPixbuf *
ws_gdk_pixbuf_new_from_resource(const char *path)
{
    GdkPixbuf   *pixbuf;
    GError      *err = NULL;

    pixbuf = gdk_pixbuf_new_from_resource(path, &err);
    g_assert_no_error(err);
    return pixbuf;
}
#endif /* HAVE_GDK_GRESOURCE */

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
