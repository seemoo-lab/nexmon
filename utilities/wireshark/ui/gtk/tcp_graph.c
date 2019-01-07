/* tcp_graph.c
 * TCP graph drawing code
 * By Pavel Mores <pvl@uh.cz>
 * Win32 port:  rwh@unifiedtech.com
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
#include <math.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if GTK_CHECK_VERSION(3,0,0)
# include <gdk/gdkkeysyms-compat.h>
#endif

#include <epan/packet.h>
#include <epan/to_str.h>
#include <epan/etypes.h>
#include <epan/ppptypes.h>
#include <epan/epan_dissect.h>
#include <epan/dissectors/packet-tcp.h>
#include <epan/address.h>
#include <epan/conv_id.h>

#include "../../globals.h"

#include <epan/stat_groups.h>
#include "ui/tap-tcp-stream.h"
#include <wsutil/utf8_entities.h>

#include "ui/gtk/gui_utils.h"
#include "ui/gtk/dlg_utils.h"
#include "ui/gtk/gui_stat_menu.h"

#include "ui/gtk/old-gtk-compat.h"

#ifndef HAVE_LRINT
#define lrint   rint
#endif

#define TCP_SYN(flags)      ( flags & TH_SYN )
#define TCP_ACK(flags)      ( flags & TH_ACK )
#define TCP_FIN(flags)      ( flags & TH_FIN )

#define TXT_WIDTH   850
#define TXT_HEIGHT  550

/* initialize_axis() */
#define AXIS_HORIZONTAL     0
#define AXIS_VERTICAL       1

#define WINDOW_TITLE_LENGTH 256

#define MOUSE_BUTTON_LEFT   1
#define MOUSE_BUTTON_MIDDLE 2
#define MOUSE_BUTTON_RIGHT  3

struct rect {
    double x, y, width, height;
};

struct line {
    double x1, y1, x2, y2;
};

struct irect {
    int x, y, width, height;
};

struct ipoint {
    int x, y;
};

struct zoomfactor {
    double x, y;
};

typedef enum {
    ELMT_NONE=0,
    ELMT_RECT=1,
    ELMT_LINE=2,
    ELMT_ELLIPSE=3
} ElementType;

struct rect_params {
    struct rect dim;
    gint filled;
};

struct line_params {
    struct line dim;
};

struct ellipse_params {
    struct rect dim;
};

struct element {
    ElementType     type;
    GdkRGBA        *elment_color_p;
    struct segment *parent;
    union {
        struct ellipse_params ellipse;
        struct rect_params    rect;
        struct line_params    line;
    } p;
};

struct element_list {
    struct element_list *next;
    struct element *elements;
};

struct axis {
    struct gtk_graph     *g;        /* which graph we belong to */
    GtkWidget        *drawing_area;
#if GTK_CHECK_VERSION(2,22,0)
    cairo_surface_t  *surface[2];
#else
    GdkPixmap        *pixmap[2];
#endif
    int               displayed;
    gboolean is_horizontal_axis;
    /* dim and orig (relative to origin of window) of axis' pixmap */
    struct irect      p;
    /* dim and orig (relative to origin of axis' pixmap) of scale itself */
    struct irect      s;
    gdouble           min, max;
    gdouble           major, minor; /* major and minor ticks */
    const char      **label;
};

#define HAXIS_INIT_HEIGHT   70
#define VAXIS_INIT_WIDTH    100
#define TITLEBAR_HEIGHT     50
#define RMARGIN_WIDTH   30

struct style_tseq_tcptrace {
    GdkRGBA seq_color;
    GdkRGBA ack_color[2];
    GdkRGBA sack_color[2];
    int flags;
};

struct style_tseq_stevens {
    int seq_width;
    int seq_height;
    int flags;
};

struct style_tput {
    int width, height;
    int ma_size;
    int flags;
};

struct style_rtt {
    int width, height;
    int flags;
};

struct style_wscale {
    int win_width;
    int win_height;
    int flags;
};

/* style flags */
#define SEQ_ORIGIN          0x1
/* show absolute sequence numbers (not differences from isn) */
#define SEQ_ORIGIN_ZERO     0x1
#define SEQ_ORIGIN_ISN      0x0
#define TIME_ORIGIN         0x10
/* show time from beginning of capture as opposed to time from beginning
 * of the connection */
#define TIME_ORIGIN_CAP     0x10
#define TIME_ORIGIN_CONN    0x0

struct cross {
    int x, y;
    int draw;           /* indicates whether we should draw cross at all */
    int erase_needed;
    GtkToggleButton *on_toggle;
    GtkToggleButton *off_toggle;
};

struct bounds {
    double x0, y0, width, height;
};

struct zoom {
    double x, y;
};

struct zooms {
    double x, y;
    double step_x, step_y;
    struct zoom initial;
#define ZOOM_OUT              (1 << 0)
#define ZOOM_HLOCK            (1 << 1)
#define ZOOM_VLOCK            (1 << 2)
#define ZOOM_STEPS_SAME       (1 << 3)
#define ZOOM_STEPS_KEEP_RATIO (1 << 4)
    int flags;
    /* unfortunately, we need them both because gtk_toggle_button_set_active()
     * with second argument FALSE doesn't do anything, somehow */
    struct {
        GtkToggleButton *in_toggle;
        GtkToggleButton *out_toggle;
        GtkEntry        *h_zoom;
        GtkEntry        *v_zoom;
        GtkSpinButton   *h_step;
        GtkSpinButton   *v_step;
    } widget;
};

struct grab {
    int grabbed;
    int x, y;
};

struct magnify {
    int            active;
    int            x, y;
    struct ipoint  offset;
    int            width, height;
    struct zoom    zoom;
    struct gtk_graph  *g;
#define MAGZOOMS_SAME       (1U << 0)
#define MAGZOOMS_SAME_RATIO (1U << 1)
#define MAGZOOMS_IGNORE     (1U << 31)
    guint flags;
    struct {
        GtkSpinButton *h_zoom, *v_zoom;
    } widget;
};

struct gtk_graph {
#define GRAPH_DESTROYED             (1 << 0)
#define GRAPH_INIT_ON_TYPE_CHANGE   (1 << 1)
    int flags;
    GtkWidget       *toplevel;    /* keypress handler needs this */
    GtkWidget       *drawing_area;
    GtkWidget       *text;        /* text widget for seg list - probably
                                  * temporary
                                  */
    PangoFontDescription *font;  /* font used for annotations etc. */
#if GTK_CHECK_VERSION(2,22,0)
    cairo_surface_t *title_surface;
    cairo_surface_t *surface[2];
#else
    GdkPixmap       *title_pixmap;
    GdkPixmap       *pixmap[2];
#endif
    int              displayed;    /* which of both pixmaps is on screen right now */
    struct {
        GtkWidget *control_panel;
        /* this belongs to style structs of graph types that make use of it */
        GtkToggleButton *time_orig_conn, *seq_orig_isn;
    } gui;
    const char     **title;

    /* Next 4 attribs describe the graph in natural units, before any scaling.
     * For example, if we want to display graph of TCP conversation that
     * started 112.309845 s after beginning of the capture and ran until
     * 479.093582 s, 237019 B went through the connection (in one direction)
     * starting with isn 31934022, then (bounds.x0, bounds.y0)=(112.309845,
     * 31934022) and (bounds.width, bounds.height)=(366.783737, 237019). */
    struct bounds    bounds;

    /* dimensions and position of the graph, both expressed already in pixels.
     * x and y give the position of upper left corner of the graph relative
     * to origin of the graph window, size is basically bounds*zoom */
    struct irect     geom;

    /* viewport (=graph window area which is reserved for graph itself), its
     * size and position relative to origin of the graph window */
    struct irect     wp;

    /* whether and where the graph has been 'grabbed' and may now be moved */
    struct grab      grab;

    /* If we need to display 237019 sequence numbers (=bytes) onto say 500
     * pixels, we have to scale the graph down by factor of 0.002109. This
     * number would be zoom.y. Obviously, both directions have separate zooms.*/
    struct zooms     zoom;

    struct cross     cross;
    gboolean         zoomrect_erase_needed;
    struct magnify   magnify;
    struct axis     *x_axis, *y_axis;

    struct tcp_graph tg;

    struct element_list *elists;        /* element lists */
    union {
        struct style_tseq_stevens  tseq_stevens;
        struct style_tseq_tcptrace tseq_tcptrace;
        struct style_tput   tput;
        struct style_rtt    rtt;
        struct style_wscale wscale;
    } s;
    /* This allows keyboard to set the radio button */
    struct {
        GtkToggleButton *graph_rtt, *graph_tput, *graph_tseqstevens, *graph_tseqttrace;
        GtkToggleButton *graph_wscale;
    } gt;
};

static int refnum=0;

#define debug(section) if (debugging & section)
/* print function entry points */
#define DBS_FENTRY          (1 << 0)
#define DBS_AXES_TICKS      (1 << 1)
#define DBS_AXES_DRAWING    (1 << 2)
#define DBS_GRAPH_DRAWING   (1 << 3)
#define DBS_TPUT_ELMTS      (1 << 4)
/*static int debugging = DBS_FENTRY;*/
static int debugging = 0;
/*static int debugging = DBS_AXES_TICKS;*/
/*static int debugging = DBS_AXES_DRAWING;*/
/*static int debugging = DBS_GRAPH_DRAWING;*/
/*static int debugging = DBS_TPUT_ELMTS;*/

static void create_gui(struct gtk_graph * );
#if 0
static void create_text_widget(struct gtk_graph * );
static void display_text(struct gtk_graph * );
#endif
static void create_drawing_area(struct gtk_graph * );
static void control_panel_create(struct gtk_graph * );
static GtkWidget *control_panel_create_zoom_group(struct gtk_graph * );
static GtkWidget *control_panel_create_magnify_group(struct gtk_graph * );
static GtkWidget *control_panel_create_cross_group(struct gtk_graph * );
static GtkWidget *control_panel_create_zoomlock_group(struct gtk_graph * );
static GtkWidget *control_panel_create_graph_type_group(struct gtk_graph * );
static void control_panel_add_zoom_page(struct gtk_graph * , GtkWidget * );
static void control_panel_add_magnify_page(struct gtk_graph * , GtkWidget * );
static void control_panel_add_origin_page(struct gtk_graph * , GtkWidget * );
static void control_panel_add_cross_page(struct gtk_graph * , GtkWidget * );
static void control_panel_add_graph_type_page(struct gtk_graph * , GtkWidget * );
static void callback_toplevel_destroy(GtkWidget * , gpointer );
static gboolean callback_delete_event(GtkWidget * , GdkEvent * , gpointer);
static void callback_close(GtkWidget * , gpointer );
static void callback_time_origin(GtkWidget * , gpointer );
static void callback_seq_origin(GtkWidget * , gpointer );
static void callback_zoomlock_h(GtkWidget * , gpointer );
static void callback_zoomlock_v(GtkWidget * , gpointer );
static void callback_zoom_inout(GtkWidget * , gpointer );
static void callback_zoom_step(GtkWidget * , gpointer );
static void callback_zoom_flags(GtkWidget * , gpointer );
static void callback_cross_on_off(GtkWidget * , gpointer );
static void callback_mag_width(GtkWidget * , gpointer );
static void callback_mag_height(GtkWidget * , gpointer );
static void callback_mag_x(GtkWidget * , gpointer );
static void callback_mag_y(GtkWidget * , gpointer );
static void callback_mag_zoom(GtkWidget * , gpointer );
static void callback_mag_flags(GtkWidget * , gpointer );
static void callback_graph_type(GtkWidget * , gpointer );
static void callback_graph_init_on_typechg(GtkWidget * , gpointer );
static void callback_create_help(GtkWidget * , gpointer );
static void get_mouse_position(GtkWidget *, int *pointer_x, int *pointer_y, GdkModifierType *mask);
static void update_zoom_spins(struct gtk_graph * );
static void graph_type_dependent_initialize(struct gtk_graph * );
static struct gtk_graph *graph_new(void);
static void graph_destroy(struct gtk_graph * );
static void graph_initialize_values(struct gtk_graph * );
static void graph_init_sequence(struct gtk_graph * );
static void draw_element_line(struct gtk_graph * , struct element * , cairo_t *cr, GdkRGBA *new_color);
static void draw_element_ellipse(struct gtk_graph * , struct element * , cairo_t *cr);
static void graph_display(struct gtk_graph * );
static void graph_pixmaps_create(struct gtk_graph * );
static void graph_pixmaps_switch(struct gtk_graph * );
static void graph_pixmap_draw(struct gtk_graph * );
static void graph_pixmap_display(struct gtk_graph * );
static void graph_element_lists_make(struct gtk_graph * );
static void graph_element_lists_free(struct gtk_graph * );
static void graph_element_lists_initialize(struct gtk_graph * );
static void graph_title_pixmap_create(struct gtk_graph * );
static void graph_title_pixmap_draw(struct gtk_graph * );
static void graph_title_pixmap_display(struct gtk_graph * );
static void graph_select_segment(struct gtk_graph * , int , int );
static int line_detect_collision(struct element * , int , int );
static int ellipse_detect_collision(struct element * , int , int );
static void axis_pixmaps_create(struct axis * );
static void axis_pixmaps_switch(struct axis * );
static void axis_display(struct axis * );
static void v_axis_pixmap_draw(struct axis * );
static void h_axis_pixmap_draw(struct axis * );
static void axis_pixmap_display(struct axis * );
static void axis_compute_ticks(struct axis * , double , double , int );
static double axis_zoom_get(struct axis * , int );
static void axis_ticks_up(int * , int * );
static void axis_ticks_down(int * , int * );
static void axis_destroy(struct axis * );
static int get_label_dim(struct axis * , int , double );
static void toggle_crosshairs(struct gtk_graph *g);
static void toggle_time_origin(struct gtk_graph * );
static void toggle_seq_origin(struct gtk_graph * );
static void restore_initial_graph_view(struct gtk_graph *g);
static void cross_draw(struct gtk_graph * , int , int );
static void cross_erase(struct gtk_graph * );
static void zoomrect_draw(struct gtk_graph * , int , int );
static void zoomrect_erase(struct gtk_graph * );
static void magnify_move(struct gtk_graph * , int , int );
static void magnify_create(struct gtk_graph * , int , int );
static void magnify_destroy(struct gtk_graph * );
static void magnify_draw(struct gtk_graph * );
static void magnify_get_geom(struct gtk_graph * , int , int );
static gboolean configure_event(GtkWidget * , GdkEventConfigure * , gpointer );
#if GTK_CHECK_VERSION(3,0,0)
static gboolean draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data);
#else
static gboolean expose_event(GtkWidget * , GdkEventExpose * , gpointer );
#endif
static gboolean button_press_event(GtkWidget * , GdkEventButton * , gpointer );
static gboolean button_release_event(GtkWidget * , GdkEventButton * , gpointer );
static gboolean motion_notify_event(GtkWidget * , GdkEventMotion * , gpointer );
static gboolean leave_notify_event(GtkWidget * , GdkEventCrossing * , gpointer );
static gboolean enter_notify_event(GtkWidget * , GdkEventCrossing * , gpointer );
static gboolean key_press_event(GtkWidget * , GdkEventKey * , gpointer );
static gboolean key_release_event(GtkWidget * , GdkEventKey * , gpointer );
static void tseq_initialize(struct gtk_graph * );
static void tseq_get_bounds(struct gtk_graph * );
static void tseq_stevens_read_config(struct gtk_graph * );
static void tseq_stevens_make_elmtlist(struct gtk_graph * );
static void tseq_stevens_toggle_seq_origin(struct gtk_graph * );
static void tseq_stevens_toggle_time_origin(struct gtk_graph * );
static void tseq_tcptrace_read_config(struct gtk_graph * );
static void tseq_tcptrace_make_elmtlist(struct gtk_graph * );
static void tseq_tcptrace_toggle_seq_origin(struct gtk_graph * );
static void tseq_tcptrace_toggle_time_origin(struct gtk_graph * );
static void tput_initialize(struct gtk_graph * );
static void tput_read_config(struct gtk_graph * );
static void tput_make_elmtlist(struct gtk_graph * );
static void tput_toggle_time_origin(struct gtk_graph * );
static void rtt_read_config(struct gtk_graph * );
static void rtt_initialize(struct gtk_graph * );
static void rtt_make_elmtlist(struct gtk_graph * );
static void rtt_toggle_seq_origin(struct gtk_graph * );
static void wscale_initialize(struct gtk_graph *);
static void wscale_read_config(struct gtk_graph *);
static void wscale_make_elmtlist(struct gtk_graph *);
#if defined(_WIN32) && !defined(__MINGW32__) && (_MSC_VER < 1800)
/* Starting VS2013, rint already defined in math.h. No need to redefine */
static int rint(double );   /* compiler template for Windows */
#endif

/* This should arguably be part of the graph, but in practice you can
   only click on one graph at a time, so this is probably OK */
static struct irect zoomrect;

/*
 * Uncomment the following define to revert Windows to
 * use original mouse button controls
 */

/* #define ORIGINAL_WIN32_BUTTONS 1 */

/* XXX - what about OS X? */
/* XXX: Needs work to ensire that the columns line up properly in both Gtk2 & Gtk3  */
/*      What is the proper way to do this ??                                        */
static char helptext[] =
    "Here's what you can do:\n"
    "\n"
#ifdef ORIGINAL_WIN32_BUTTONS
    "   <Ctrl>-Left Mouse Button		selects segment under cursor in Wireshark's packet list\n"
    "\n"
    "   Left Mouse Button			zooms in (towards area under mouse pointer)\n"
    "   <Shift>-Left Mouse Button		zooms out\n"
    "\n"
    "   Right Mouse Button			moves the graph (if zoomed in)\n"
    "   <Ctrl>-Right Mouse Button		displays a portion of graph under cursor magnified\n"
#else /* !ORIGINAL_WIN32_BUTTONS */
    "   Left Mouse Button			selects segment under cursor in Wireshark's packet list\n"
    "								can also drag to zoom in on a rectangular region\n"
    "\n"
    "   Middle Mouse Button			zooms in (towards area under cursor)\n"
    "   <Shift>-Middle Mouse Button	zooms out\n"
    "\n"
    "   Right Mouse Button			moves the graph (if zoomed in)\n"
    "   <Ctrl>-Right Mouse Button		displays a portion of graph under cursor magnified\n"
#endif
    "\n"
    "\n"
    "   '1'				display Round Trip Time Graph\n"
    "   '2'				display Throughput Graph\n"
    "   '3'				display Time/Sequence Graph (Stevens)\n"
    "   '4'				display Time/Sequence Graph (tcptrace)\n"
    "   '5'				display Window Scaling Graph\n"
    "\n"
    "   <Space bar>	toggles crosshairs on/off\n"
    "\n"
    "   'i' or '+'			zoom in (towards area under mouse pointer)\n"
    "   'o' or '-'			zoom out\n"
    "   'r' or <Home>	restore graph to initial state (zoom out max)\n"
    "   's'				toggles relative/absolute sequence numbers\n"
    "   't'				toggles time origin\n"
    "   'g'				go to frame under cursor in Wireshark's packet list (if possible)\n"
    "\n"
    "   <Left>			move view left by 100 pixels (if zoomed in)\n"
    "   <Right>		move view right 100 pixels (if zoomed in)\n"
    "   <Up>			move view up by 100 pixels (if zoomed in)\n"
    "   <Down>		move view down by 100 pixels (if zoomed in)\n"
    "\n"
    "   <Shift><Left>	move view left by 10 pixels (if zoomed in)\n"
    "   <Shift><Right>	move view right 10 pixels (if zoomed in)\n"
    "   <Shift><Up>	move view up by 10 pixels (if zoomed in)\n"
    "   <Shift><Down>	move view down by 10 pixels (if zoomed in)\n"
    "\n"
    "   <Ctrl><Left>	move view left by 1 pixel (if zoomed in)\n"
    "   <Ctrl><Right>	move view right 1 pixel (if zoomed in)\n"
    "   <Ctrl><Up>	move view up by 1 pixel (if zoomed in)\n"
    "   <Ctrl><Down>	move view down by 1 pixel (if zoomed in)\n"
;

#if 0
static void debug_coord(struct gtk_graph *g, const char *c)
{
    static guint count = 0;

    count++;
    printf("%u: %s\n", count, c);
    printf("%u:  g->geom.width %d\n", count, g->geom.width);
    printf("%u: g->geom.height %d\n", count, g->geom.height);
    printf("%u:      g->geom.x %d\n", count, g->geom.x);
    printf("%u:      g->geom.y %d\n", count, g->geom.y);

    printf("%u:    g->wp.width %d\n", count, g->wp.width);
    printf("%u:   g->wp.height %d\n", count, g->wp.height);
    printf("%u:        g->wp.x %d\n", count, g->wp.x);
    printf("%u:        g->wp.y %d\n", count, g->wp.y);
    printf("---------------\n");
}
#endif

static void set_busy_cursor(GdkWindow *w)
{
    GdkCursor *cursor;

    cursor = gdk_cursor_new(GDK_WATCH);
    gdk_window_set_cursor(w, cursor);
    gdk_flush();
#if GTK_CHECK_VERSION(3,0,0)
    g_object_unref(cursor);
#else
    gdk_cursor_unref(cursor);
#endif
}

static void unset_busy_cursor(GdkWindow *w, gboolean cross)
{
    GdkCursor *cursor;

    if (cross) {
        cursor = gdk_cursor_new(GDK_CROSSHAIR);
        gdk_window_set_cursor(w, cursor);
        gdk_flush();
#if GTK_CHECK_VERSION(3,0,0)
        g_object_unref(cursor);
#else
        gdk_cursor_unref(cursor);
#endif
    } else {
        gdk_window_set_cursor(w, NULL);
        gdk_flush();
    }
}

void tcp_graph_cb(GtkAction *action, gpointer user_data _U_)
{
    struct segment  current;
    struct gtk_graph   *g;
    const  gchar   *name;
    tcp_graph_type  graph_type;

    name = gtk_action_get_name(action);
    if (strcmp(name, "/Statistics/TCPStreamGraphMenu/Time-Sequence-Graph-Stevens") == 0) {
        graph_type = GRAPH_TSEQ_STEVENS;
    } else if (strcmp(name, "/Statistics/TCPStreamGraphMenu/Time-Sequence-Graph-tcptrace") == 0) {
        graph_type = GRAPH_TSEQ_TCPTRACE;
    } else if (strcmp(name, "/Statistics/TCPStreamGraphMenu/Throughput-Graph") == 0) {
        graph_type = GRAPH_THROUGHPUT;
    } else if (strcmp(name, "/Statistics/TCPStreamGraphMenu/RTT-Graph") == 0) {
        graph_type = GRAPH_RTT;
    } else if (strcmp(name, "/Statistics/TCPStreamGraphMenu/Window-Scaling-Graph") == 0) {
        graph_type = GRAPH_WSCALE;
    } else {
        return;
    }

    debug(DBS_FENTRY) puts("tcp_graph_cb()");

    if (!select_tcpip_session(&cfile, &current)) {
        return;
    }

    if (! (g = graph_new()))
        return;

    refnum++;
    graph_initialize_values(g);

    g->tg.type = graph_type;

    graph_segment_list_get(&cfile, &g->tg, FALSE);
    create_gui(g);
    /* display_text(g); */
    graph_init_sequence(g);

}

void tcp_graph_known_stream_launch(address *src_address, guint16 src_port,
                                   address *dst_address, guint16 dst_port,
                                   conv_id_t stream)
{
    struct gtk_graph *g;

    if (!(g = graph_new())) {
        return;
    }

    refnum++;
    graph_initialize_values(g);

    /* Can set stream info for graph now */
    copy_address(&g->tg.src_address, src_address);
    g->tg.src_port = src_port;
    copy_address(&g->tg.dst_address, dst_address);
    g->tg.dst_port = dst_port;
    g->tg.stream = stream;

    /* This graph type is arguably the most useful, so start there */
    g->tg.type = GRAPH_TSEQ_TCPTRACE;

    /* Get our list of segments from the packet list */
    graph_segment_list_get(&cfile, &g->tg, TRUE);

    create_gui(g);
    graph_init_sequence(g);
}


static void create_gui(struct gtk_graph *g)
{
    /* ToDo: Ensure that drawing area window doesn't
     *       (completely) cover the contraol_panel window.
     */
    debug(DBS_FENTRY) puts("create_gui()");
    /* create_text_widget(g); */
    control_panel_create(g);
    create_drawing_area(g);
}



static void create_drawing_area(struct gtk_graph *g)
{
#if GTK_CHECK_VERSION(3,0,0)
    GtkStyleContext *context;
#endif
    char        *display_name;
    char         window_title[WINDOW_TITLE_LENGTH];
    GtkAllocation    widget_alloc;
    char *src_addr, *dst_addr;
#if 0
    /* Prep. to include the controls in the graph window */
    GtkWidget *frame;
    GtkWidget *vbox;
    GtkWidget *hbox;
#endif
    debug(DBS_FENTRY) puts("create_drawing_area()");

    /* Set title of window with file + conversation details */
    display_name = cf_get_display_name(&cfile);
    src_addr = (char*)address_to_str(NULL, &g->tg.src_address);
    dst_addr = (char*)address_to_str(NULL, &g->tg.dst_address);
    g_snprintf(window_title, WINDOW_TITLE_LENGTH, "TCP Graph %d: %s %s:%d " UTF8_RIGHTWARDS_ARROW " %s:%d",
               refnum,
               display_name,
               src_addr,
               g->tg.src_port,
               dst_addr,
               g->tg.dst_port
    );
    g_free(display_name);
    wmem_free(NULL, src_addr);
    wmem_free(NULL, dst_addr);
    g->toplevel = dlg_window_new("Tcp Graph");
    gtk_window_set_title(GTK_WINDOW(g->toplevel), window_title);
    gtk_widget_set_name(g->toplevel, "Test Graph");

    /* Create the drawing area */
    g->drawing_area = gtk_drawing_area_new();
    g->x_axis->drawing_area = g->y_axis->drawing_area = g->drawing_area;
    gtk_widget_set_size_request(g->drawing_area,
                                g->wp.width + g->wp.x + RMARGIN_WIDTH,
                                g->wp.height + g->wp.y + g->x_axis->s.height);
    gtk_widget_show(g->drawing_area);

#if GTK_CHECK_VERSION(3,0,0)
    g_signal_connect(g->drawing_area, "draw", G_CALLBACK(draw_event), g);
#else
    g_signal_connect(g->drawing_area, "expose_event", G_CALLBACK(expose_event), g);
#endif
    /* this has to be done later, after the widget has been shown */
    /*
    g_signal_connect(g->drawing_area, "configure_event", G_CALLBACK(configure_event), g);
     */

    g_signal_connect(g->drawing_area, "button_press_event",
                     G_CALLBACK(button_press_event), g);
    g_signal_connect(g->drawing_area, "button_release_event",
                     G_CALLBACK(button_release_event), g);
    g_signal_connect(g->drawing_area, "motion_notify_event",
                     G_CALLBACK(motion_notify_event), g);
    g_signal_connect(g->drawing_area, "leave_notify_event",
                     G_CALLBACK(leave_notify_event), g);
    g_signal_connect(g->drawing_area, "enter_notify_event",
                     G_CALLBACK(enter_notify_event), g);
    g_signal_connect(g->toplevel, "destroy", G_CALLBACK(callback_toplevel_destroy), g);
    /* why doesn't drawing area send key_press_signals? */
    g_signal_connect(g->toplevel, "key_press_event", G_CALLBACK(key_press_event), g);
    g_signal_connect(g->toplevel, "key_release_event", G_CALLBACK(key_release_event),
                     g);
    gtk_widget_set_events(g->toplevel, GDK_KEY_PRESS_MASK|GDK_KEY_RELEASE_MASK);

    gtk_widget_set_events(g->drawing_area,
                          GDK_EXPOSURE_MASK
                          | GDK_LEAVE_NOTIFY_MASK
                          | GDK_ENTER_NOTIFY_MASK
                          | GDK_BUTTON_PRESS_MASK
                          | GDK_BUTTON_RELEASE_MASK
                          | GDK_POINTER_MOTION_MASK
                          | GDK_POINTER_MOTION_HINT_MASK);

#if 0
    /* Prep. to include the controls in the graph window */

    vbox = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 0, FALSE);
    gtk_container_add(GTK_CONTAINER(g->toplevel), vbox);
    gtk_container_set_border_width(GTK_CONTAINER(g->toplevel), 5);
    gtk_widget_show(vbox);

    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
    gtk_container_add(GTK_CONTAINER(frame), g->drawing_area);
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
    gtk_widget_show(frame);


    /*gtk_box_pack_start(GTK_BOX(vbox), g->gui.control_panel, FALSE, FALSE, 0);*/

    hbox = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 3);
    gtk_widget_show(hbox);

    create_ctrl_area(g, hbox);

#endif

    gtk_container_add(GTK_CONTAINER(g->toplevel), g->drawing_area);
    gtk_widget_show(g->toplevel);

    /* in case we didn't get what we asked for */
    gtk_widget_get_allocation(GTK_WIDGET(g->drawing_area), &widget_alloc);
    g->wp.width = widget_alloc.width - g->wp.x - RMARGIN_WIDTH;
    g->wp.height = widget_alloc.height - g->wp.y - g->x_axis->s.height;

#if GTK_CHECK_VERSION(3,0,0)
    context = gtk_widget_get_style_context(g->drawing_area);
    gtk_style_context_get(context, GTK_STATE_FLAG_NORMAL,
                          GTK_STYLE_PROPERTY_FONT, &g->font,
                          NULL);
#else
    g->font = gtk_widget_get_style(g->drawing_area)->font_desc;


    /* this is probably quite an ugly way to get rid of the first configure
     * event
     * immediately after gtk_widget_show(window) drawing_area gets a configure
     * event which is handled during the next return to gtk_main which is
     * probably the gdk_gc_new() call. configure handler calls
     * graph_element_lists_make() which is not good because the graph struct is
     * not fully set up yet - namely we're not sure about actual geometry
     * and we don't have the GC's at all. so we just postpone installation
     * of configure handler until we're ready to deal with it.
     *
     * !!! NEMLLO BY TO BYT NA KONCI graph_init_sequence()? !!!
     *
     */
#endif
    g_signal_connect(g->drawing_area, "configure_event", G_CALLBACK(configure_event),
                     g);

    /* puts("exiting create_drawing_area()"); */
}

static void callback_toplevel_destroy(GtkWidget *widget _U_, gpointer data)
{
    struct gtk_graph *g = (struct gtk_graph *)data;

    if (!(g->flags & GRAPH_DESTROYED)) {
        g->flags |= GRAPH_DESTROYED;
        graph_destroy((struct gtk_graph *)data);
    }
}

static void control_panel_create(struct gtk_graph *g)
{
    GtkWidget *toplevel, *notebook;
    GtkWidget *top_vb;
    GtkWidget *help_bt, *close_bt, *bbox;
    char window_title[WINDOW_TITLE_LENGTH];

    debug(DBS_FENTRY) puts("control_panel_create()");

    notebook = gtk_notebook_new();
    control_panel_add_zoom_page(g, notebook);
    control_panel_add_magnify_page(g, notebook);
    control_panel_add_origin_page(g, notebook);
    control_panel_add_cross_page(g, notebook);
    control_panel_add_graph_type_page(g, notebook);

    g_snprintf(window_title, WINDOW_TITLE_LENGTH,
               "Graph %d - Control - Wireshark", refnum);
    toplevel = dlg_window_new("tcp-graph-control");
    gtk_window_set_title(GTK_WINDOW(toplevel), window_title);

    gtk_window_set_resizable(GTK_WINDOW(toplevel), FALSE); /* XXX: Acceptable ? */

    top_vb = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 0, FALSE);
    gtk_container_add(GTK_CONTAINER(toplevel), top_vb);

    gtk_box_pack_start(GTK_BOX(top_vb), notebook, FALSE, FALSE, 5);

    /* Button row. */
    bbox = dlg_button_row_new(GTK_STOCK_HELP, GTK_STOCK_CLOSE, NULL);
        gtk_box_pack_start(GTK_BOX(top_vb), bbox, FALSE, FALSE, 5);

    help_bt = (GtkWidget *)g_object_get_data(G_OBJECT(bbox), GTK_STOCK_HELP);
    g_signal_connect(help_bt, "clicked", G_CALLBACK(callback_create_help), g);

    close_bt = (GtkWidget *)g_object_get_data(G_OBJECT(bbox), GTK_STOCK_CLOSE);
    window_set_cancel_button(toplevel, close_bt, NULL);
    g_signal_connect(close_bt, "clicked",      G_CALLBACK(callback_close), g);

    g_signal_connect(toplevel, "delete_event", G_CALLBACK(callback_delete_event), g);
    g_signal_connect(toplevel, "destroy",      G_CALLBACK(callback_toplevel_destroy), g);

    gtk_widget_show_all(toplevel);
    window_present(toplevel);

    g->gui.control_panel = toplevel;
}

static void control_panel_add_zoom_page(struct gtk_graph *g, GtkWidget *n)
{
    GtkWidget *zoom_frame;
    GtkWidget *zoom_lock_frame;
    GtkWidget *label;
    GtkWidget *box;

    zoom_frame = control_panel_create_zoom_group(g);
    gtk_container_set_border_width(GTK_CONTAINER(zoom_frame), 5);
    zoom_lock_frame = control_panel_create_zoomlock_group(g);
    gtk_container_set_border_width(GTK_CONTAINER(zoom_lock_frame), 5);
    box = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 0, FALSE);
    gtk_box_pack_start(GTK_BOX(box), zoom_frame,      TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), zoom_lock_frame, TRUE, TRUE, 0);
    gtk_widget_show(box);
    label = gtk_label_new("Zoom");
    gtk_notebook_append_page(GTK_NOTEBOOK(n), box, label);
}

static void control_panel_add_magnify_page(struct gtk_graph *g, GtkWidget *n)
{
    GtkWidget *mag_frame, *label;

    mag_frame = control_panel_create_magnify_group(g);
    gtk_container_set_border_width(GTK_CONTAINER(mag_frame), 5);
    label = gtk_label_new("Magnify");
    gtk_notebook_append_page(GTK_NOTEBOOK(n), mag_frame, label);
}

static void control_panel_add_origin_page(struct gtk_graph *g, GtkWidget *n)
{
    GtkWidget *time_orig_cap, *time_orig_conn, *time_orig_box, *time_orig_frame;
    GtkWidget *seq_orig_isn, *seq_orig_zero, *seq_orig_box, *seq_orig_frame;
    GtkWidget *box, *label;

    /* time origin box */
    time_orig_cap  = gtk_radio_button_new_with_label(NULL, "beginning of capture");
    time_orig_conn = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(time_orig_cap)),
        "beginning of this TCP connection");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(time_orig_conn), TRUE);
    time_orig_box = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 0, FALSE);
    gtk_box_pack_start(GTK_BOX(time_orig_box), time_orig_conn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(time_orig_box), time_orig_cap, TRUE, TRUE, 0);
    time_orig_frame = gtk_frame_new("Time origin");
    gtk_container_set_border_width(GTK_CONTAINER(time_orig_frame), 5);
    gtk_container_add(GTK_CONTAINER(time_orig_frame), time_orig_box);

    /* sequence number origin group */
    seq_orig_isn  = gtk_radio_button_new_with_label(NULL, "initial sequence number");
    seq_orig_zero = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(seq_orig_isn)),
        "0 (=absolute)");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(seq_orig_isn), TRUE);
    seq_orig_box = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 0, FALSE);
    gtk_box_pack_start(GTK_BOX(seq_orig_box), seq_orig_isn,  TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(seq_orig_box), seq_orig_zero, TRUE, TRUE, 0);
    seq_orig_frame = gtk_frame_new("Sequence number origin");
    gtk_container_set_border_width(GTK_CONTAINER(seq_orig_frame), 5);
    gtk_container_add(GTK_CONTAINER(seq_orig_frame), seq_orig_box);

    g->gui.time_orig_conn = (GtkToggleButton * )time_orig_conn;
    g->gui.seq_orig_isn   = (GtkToggleButton * )seq_orig_isn;

    g_signal_connect(time_orig_conn, "toggled", G_CALLBACK(callback_time_origin), g);
    g_signal_connect(seq_orig_isn,   "toggled", G_CALLBACK(callback_seq_origin), g);

    box = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 0, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(box), 5);
    gtk_box_pack_start(GTK_BOX(box), time_orig_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), seq_orig_frame,  TRUE, TRUE, 0);
    gtk_widget_show(box);
    label = gtk_label_new("Origin");
    gtk_notebook_append_page(GTK_NOTEBOOK(n), box, label);
}

static void control_panel_add_cross_page (struct gtk_graph *g, GtkWidget *n)
{
    GtkWidget *cross_frame, *label;

    cross_frame = control_panel_create_cross_group(g);
    gtk_container_set_border_width(GTK_CONTAINER(cross_frame), 5);
    label = gtk_label_new("Cross");
    gtk_notebook_append_page(GTK_NOTEBOOK(n), cross_frame, label);
}

static void control_panel_add_graph_type_page(struct gtk_graph *g, GtkWidget *n)
{
    GtkWidget *frame, *label;

    frame = control_panel_create_graph_type_group(g);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 5);
    label = gtk_label_new("Graph type");
    gtk_notebook_append_page(GTK_NOTEBOOK(n), frame, label);
}

/* Treat this as a cancel, by calling "callback_close()" */
static gboolean
callback_delete_event(GtkWidget *widget _U_, GdkEvent *event _U_,
                      gpointer data)
{
    callback_close(NULL, data);
    return FALSE;
}

static void callback_close(GtkWidget *widget _U_, gpointer data)
{
    struct gtk_graph *g = (struct gtk_graph * )data;

    if (!(g->flags & GRAPH_DESTROYED)) {
        g->flags |= GRAPH_DESTROYED;
        graph_destroy((struct gtk_graph * )data);
    }
}

static void callback_create_help(GtkWidget *widget _U_, gpointer data _U_)
{
    GtkWidget     *toplevel, *vbox, *text, *scroll, *bbox, *close_bt;
    GtkTextBuffer *buf;

    toplevel = dlg_window_new("Help for TCP graphing");
    gtk_window_set_default_size(GTK_WINDOW(toplevel), 500, 400);

    vbox = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 0, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    gtk_container_add(GTK_CONTAINER(toplevel), vbox);

    scroll = scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scroll),
                                        GTK_SHADOW_IN);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);
    text = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text), FALSE);
    buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
    gtk_text_buffer_set_text(buf, helptext, -1);
    gtk_container_add(GTK_CONTAINER(scroll), text);

    /* Button row. */
    bbox = dlg_button_row_new(GTK_STOCK_CLOSE, NULL);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 0);
    gtk_widget_show(bbox);

    close_bt = (GtkWidget *)g_object_get_data(G_OBJECT(bbox), GTK_STOCK_CLOSE);
    window_set_cancel_button(toplevel, close_bt, window_cancel_button_cb);

    g_signal_connect(toplevel, "delete_event", G_CALLBACK(window_delete_event_cb), NULL);

    gtk_widget_show_all(toplevel);
    window_present(toplevel);
}

static void get_mouse_position(GtkWidget *widget, int *pointer_x, int *pointer_y, GdkModifierType *mask)
{
#if GTK_CHECK_VERSION(3,0,0)
    gdk_window_get_device_position(gtk_widget_get_window(widget),
                                   gdk_device_manager_get_client_pointer(
                                       gdk_display_get_device_manager(
                                           gtk_widget_get_display(GTK_WIDGET(widget)))),
                                   pointer_x, pointer_y, mask);

#else
    gdk_window_get_pointer(gtk_widget_get_window(widget), pointer_x, pointer_y, mask);
#endif

}

static void callback_time_origin(GtkWidget *toggle _U_, gpointer data)
{
    toggle_time_origin((struct gtk_graph * )data);
}

static void callback_seq_origin(GtkWidget *toggle _U_, gpointer data)
{
    toggle_seq_origin((struct gtk_graph * )data);
}

static GtkWidget *control_panel_create_zoom_group(struct gtk_graph *g)
{
    GtkWidget     *zoom_in, *zoom_out, *zoom_box, *zoom_frame;
    GtkAdjustment *zoom_h_adj, *zoom_v_adj;
    GtkWidget     *zoom_inout_box, *zoom_h_step_label, *zoom_h_step;
    GtkWidget     *zoom_v_step_label, *zoom_v_step;
    GtkWidget     *zoom_separator1, *zoom_separator2, *zoom_step_grid, *zoom_grid;
    GtkWidget     *zoom_ratio_toggle, *zoom_same_toggle;
    GtkWidget     *zoom_h_entry, *zoom_v_entry;
    GtkWidget     *zoom_h_label, *zoom_v_label;

    zoom_in  = gtk_radio_button_new_with_label(NULL, "in");
    zoom_out = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(zoom_in)), "out");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(zoom_in), TRUE);
    zoom_inout_box = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0, FALSE);
    gtk_box_pack_start(GTK_BOX(zoom_inout_box), zoom_in, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(zoom_inout_box), zoom_out, FALSE, FALSE, 0);

    zoom_separator1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

    zoom_h_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(zoom_h_entry), "1.000");
    gtk_editable_set_editable(GTK_EDITABLE(zoom_h_entry), FALSE);
    zoom_h_label = gtk_label_new("Horizontal:");

    zoom_v_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(zoom_v_entry), "1.000");
    gtk_editable_set_editable(GTK_EDITABLE(zoom_v_entry), FALSE);
    zoom_v_label = gtk_label_new("Vertical:");

    g->zoom.widget.h_zoom = (GtkEntry * )zoom_h_entry;
    g->zoom.widget.v_zoom = (GtkEntry * )zoom_v_entry;

    zoom_grid = ws_gtk_grid_new();
    ws_gtk_grid_attach_extended(GTK_GRID(zoom_grid), zoom_h_label, 0, 0, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(zoom_grid), zoom_h_entry, 1, 0, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(zoom_grid), zoom_v_label, 0, 1, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(zoom_grid), zoom_v_entry, 1, 1, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);

    zoom_separator2 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);

    zoom_h_adj  = (GtkAdjustment * )gtk_adjustment_new((gfloat)1.2, 1.0, 5, (gfloat)0.1, 1, 0);
    zoom_h_step = gtk_spin_button_new(zoom_h_adj, 0, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(zoom_h_step), TRUE);
    zoom_h_step_label = gtk_label_new("Horizontal step:");

    zoom_v_adj  = (GtkAdjustment * )gtk_adjustment_new((gfloat)1.2, 1.0, 5, (gfloat)0.1, 1, 0);
    zoom_v_step = gtk_spin_button_new(zoom_v_adj, 0, 1);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(zoom_v_step), TRUE);
    zoom_v_step_label = gtk_label_new("Vertical step:");

    g->zoom.widget.h_step = (GtkSpinButton * )zoom_h_step;
    g->zoom.widget.v_step = (GtkSpinButton * )zoom_v_step;

    zoom_same_toggle  = gtk_check_button_new_with_label("Keep them the same");
    zoom_ratio_toggle = gtk_check_button_new_with_label("Preserve their ratio");
    g_object_set_data(G_OBJECT(zoom_same_toggle), "flag", (gpointer)ZOOM_STEPS_SAME);
    g_object_set_data(G_OBJECT(zoom_ratio_toggle), "flag",
                      (gpointer)ZOOM_STEPS_KEEP_RATIO);
    g_signal_connect(zoom_same_toggle, "clicked", G_CALLBACK(callback_zoom_flags), g);
    g_signal_connect(zoom_ratio_toggle, "clicked", G_CALLBACK(callback_zoom_flags), g);

    zoom_step_grid = ws_gtk_grid_new();
    ws_gtk_grid_attach_extended(GTK_GRID(zoom_step_grid), zoom_h_step_label, 0, 0, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(zoom_step_grid), zoom_h_step, 1, 0, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(zoom_step_grid), zoom_v_step_label, 0, 1, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(zoom_step_grid), zoom_v_step, 1, 1, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(zoom_step_grid), zoom_same_toggle, 0, 2, 2, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(zoom_step_grid), zoom_ratio_toggle, 0, 3, 2, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);

    zoom_box = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 0, FALSE);
    gtk_box_pack_start(GTK_BOX(zoom_box), zoom_inout_box,  TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(zoom_box), zoom_separator1, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(zoom_box), zoom_grid,       TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(zoom_box), zoom_separator2, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(zoom_box), zoom_step_grid,  TRUE, TRUE, 0);
    zoom_frame = gtk_frame_new("Zoom");
    gtk_container_add(GTK_CONTAINER(zoom_frame), zoom_box);

    g_object_set_data(G_OBJECT(zoom_h_step), "direction", GINT_TO_POINTER(0));
    g_object_set_data(G_OBJECT(zoom_v_step), "direction", GINT_TO_POINTER(1));

    g_signal_connect(zoom_in, "toggled",     G_CALLBACK(callback_zoom_inout), g);
    g_signal_connect(zoom_h_step, "changed", G_CALLBACK(callback_zoom_step),  g);
    g_signal_connect(zoom_v_step, "changed", G_CALLBACK(callback_zoom_step),  g);

    g->zoom.widget.in_toggle  = (GtkToggleButton * )zoom_in;
    g->zoom.widget.out_toggle = (GtkToggleButton * )zoom_out;
    return zoom_frame;
}

static void callback_zoom_inout(GtkWidget *toggle, gpointer data)
{
    struct gtk_graph *g = (struct gtk_graph * )data;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle)))
        g->zoom.flags &= ~ZOOM_OUT;
    else
        g->zoom.flags |= ZOOM_OUT;
}

static void callback_zoom_step(GtkWidget *spin, gpointer data)
{
    struct gtk_graph  *g = (struct gtk_graph * )data;
    double         value;
    int            direction;
    double        *zoom_this, *zoom_other;
    GtkSpinButton *widget_this, *widget_other;
    double         old_this;

    direction = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(spin), "direction"));
    value     = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));

    if (direction) {
        zoom_this    = &g->zoom.step_y;
        zoom_other   = &g->zoom.step_x;
        widget_this  =  g->zoom.widget.v_step;
        widget_other =  g->zoom.widget.h_step;
    } else {
        zoom_this    = &g->zoom.step_x;
        zoom_other   = &g->zoom.step_y;
        widget_this  =  g->zoom.widget.h_step;
        widget_other =  g->zoom.widget.v_step;
    }

    old_this   = *zoom_this;
    *zoom_this = value;
    if (g->zoom.flags & ZOOM_STEPS_SAME) {
        *zoom_other = value;
        gtk_spin_button_set_value(widget_other, (gfloat) *zoom_other);
    } else if (g->zoom.flags & ZOOM_STEPS_KEEP_RATIO) {
        double old_other = *zoom_other;
        *zoom_other *= value / old_this;
        if (*zoom_other < 1.0) {
            *zoom_other = 1.0;
            *zoom_this  = old_this * 1.0 / old_other;
            gtk_spin_button_set_value(widget_this, (gfloat) *zoom_this);
        } else if (*zoom_other > 5.0) {
            *zoom_other = 5.0;
            *zoom_this  = old_this * 5.0 / old_other;
            gtk_spin_button_set_value(widget_this, (gfloat) *zoom_this);
        }
        gtk_spin_button_set_value(widget_other, (gfloat) *zoom_other);
    }
}

static void callback_zoom_flags(GtkWidget *toggle, gpointer data)
{
    struct gtk_graph *g = (struct gtk_graph * )data;
    int flag = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(toggle), "flag"));

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle)))
        g->zoom.flags |= flag;
    else
        g->zoom.flags &= ~flag;
}

static void update_zoom_spins(struct gtk_graph *g)
{
    char s[32];

    g_snprintf(s, sizeof(s), "%.3f", g->zoom.x / g->zoom.initial.x);
    gtk_entry_set_text(g->zoom.widget.h_zoom, s);
    g_snprintf(s, sizeof(s), "%.3f", g->zoom.y / g->zoom.initial.y);
    gtk_entry_set_text(g->zoom.widget.v_zoom, s);
}

static GtkWidget *control_panel_create_magnify_group(struct gtk_graph *g)
{
    GtkWidget     *mag_width_label, *mag_width;
    GtkWidget     *mag_height_label, *mag_height;
    GtkWidget     *mag_x_label, *mag_x;
    GtkWidget     *mag_y_label, *mag_y;
    GtkWidget     *mag_wh_grid, *mag_zoom_frame, *mag_zoom_grid;
    GtkWidget     *mag_h_zoom_label, *mag_h_zoom;
    GtkWidget     *mag_v_zoom_label, *mag_v_zoom;
    GtkWidget     *mag_zoom_same, *mag_zoom_ratio;
    GtkAdjustment *mag_width_adj, *mag_height_adj, *mag_x_adj, *mag_y_adj;
    GtkAdjustment *mag_h_zoom_adj, *mag_v_zoom_adj;
    GtkWidget     *mag_box, *mag_frame;

    mag_width_label  = gtk_label_new("Width:");
    mag_width_adj    = (GtkAdjustment * )gtk_adjustment_new(250, 100, 600, 1, 10, 0);
    mag_width        = gtk_spin_button_new(mag_width_adj, 0, 0);

    mag_height_label = gtk_label_new("Height:");
    mag_height_adj   = (GtkAdjustment * )gtk_adjustment_new(250, 100, 600, 1, 10, 0);
    mag_height       = gtk_spin_button_new(mag_height_adj, 0, 0);

    mag_x_label = gtk_label_new("X:");
    mag_x_adj   = (GtkAdjustment * )gtk_adjustment_new(0, -1000, 1000, 1, 10, 0);
    mag_x = gtk_spin_button_new(mag_x_adj, 0, 0);

    mag_y_label = gtk_label_new("Y:");
    mag_y_adj   = (GtkAdjustment * )gtk_adjustment_new(0, -1000, 1000, 1, 10, 0);
    mag_y = gtk_spin_button_new(mag_y_adj, 0, 0);

    mag_wh_grid = ws_gtk_grid_new();
    ws_gtk_grid_attach_extended(GTK_GRID(mag_wh_grid), mag_width_label, 0, 0, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(mag_wh_grid), mag_width, 1, 0, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(mag_wh_grid), mag_height_label, 0, 1, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(mag_wh_grid), mag_height, 1, 1, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(mag_wh_grid), mag_x_label, 0, 2, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(mag_wh_grid), mag_x, 1, 2, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(mag_wh_grid), mag_y_label, 0, 3, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(mag_wh_grid), mag_y, 1, 3, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 5, 0);

    mag_h_zoom_label = gtk_label_new("Horizontal:");
    mag_h_zoom_adj   = (GtkAdjustment *)gtk_adjustment_new(10.0, 1.0, 25.0, (gfloat)0.1, 1, 0);
    mag_h_zoom = gtk_spin_button_new(mag_h_zoom_adj, 0, 1);

    mag_v_zoom_label = gtk_label_new("Vertical:");
    mag_v_zoom_adj   = (GtkAdjustment *)gtk_adjustment_new(10.0, 1.0, 25.0, (gfloat)0.1, 1, 0);
    mag_v_zoom       = gtk_spin_button_new(mag_v_zoom_adj, 0, 1);

    mag_zoom_same    = gtk_check_button_new_with_label("Keep them the same");
    mag_zoom_ratio   = gtk_check_button_new_with_label("Preserve their ratio");

    mag_zoom_grid = ws_gtk_grid_new();
    ws_gtk_grid_attach_extended(GTK_GRID(mag_zoom_grid), mag_h_zoom_label, 0, 0, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 0, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(mag_zoom_grid), mag_h_zoom, 1, 0, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 0, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(mag_zoom_grid), mag_v_zoom_label, 0, 1 , 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 0, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(mag_zoom_grid), mag_v_zoom, 1, 1, 1, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 0, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(mag_zoom_grid), mag_zoom_same, 0, 2, 2, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 0, 0);
    ws_gtk_grid_attach_extended(GTK_GRID(mag_zoom_grid), mag_zoom_ratio, 0, 3, 2, 1,
                                (GtkAttachOptions)(GTK_FILL|GTK_EXPAND), (GtkAttachOptions)0, 0, 0);

    mag_zoom_frame = gtk_frame_new("Magnify zoom");
    gtk_container_add(GTK_CONTAINER(mag_zoom_frame), mag_zoom_grid);
    gtk_container_set_border_width(GTK_CONTAINER(mag_zoom_frame), 3);

    mag_box = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 0, FALSE);
    gtk_box_pack_start(GTK_BOX(mag_box), mag_wh_grid,    TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(mag_box), mag_zoom_frame, TRUE, TRUE, 0);
    mag_frame = gtk_frame_new("Magnify");
    gtk_container_add(GTK_CONTAINER(mag_frame), mag_box);

    g->magnify.widget.h_zoom = (GtkSpinButton * )mag_h_zoom;
    g->magnify.widget.v_zoom = (GtkSpinButton * )mag_v_zoom;
    g_object_set_data(G_OBJECT(mag_h_zoom), "direction", GINT_TO_POINTER(0));
    g_object_set_data(G_OBJECT(mag_v_zoom), "direction", GINT_TO_POINTER(1));
    g_object_set_data(G_OBJECT(mag_zoom_same),  "flag", (gpointer)MAGZOOMS_SAME);
    g_object_set_data(G_OBJECT(mag_zoom_ratio), "flag", (gpointer)MAGZOOMS_SAME_RATIO);

    g_signal_connect(mag_width, "changed",      G_CALLBACK(callback_mag_width),  g);
    g_signal_connect(mag_height, "changed",     G_CALLBACK(callback_mag_height), g);
    g_signal_connect(mag_x, "changed",          G_CALLBACK(callback_mag_x),      g);
    g_signal_connect(mag_y, "changed",          G_CALLBACK(callback_mag_y),      g);
    g_signal_connect(mag_h_zoom, "changed",     G_CALLBACK(callback_mag_zoom),   g);
    g_signal_connect(mag_v_zoom, "changed",     G_CALLBACK(callback_mag_zoom),   g);
    g_signal_connect(mag_zoom_same, "clicked",  G_CALLBACK(callback_mag_flags),  g);
    g_signal_connect(mag_zoom_ratio, "clicked", G_CALLBACK(callback_mag_flags),  g);

    return mag_frame;
}

static void callback_mag_width(GtkWidget *spin, gpointer data)
{
    struct gtk_graph *g = (struct gtk_graph * )data;

    g->magnify.width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
}

static void callback_mag_height(GtkWidget *spin, gpointer data)
{
    struct gtk_graph *g = (struct gtk_graph * )data;

    g->magnify.height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
}

static void callback_mag_x(GtkWidget *spin, gpointer data)
{
    struct gtk_graph *g = (struct gtk_graph * )data;

    g->magnify.offset.x = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
}

static void callback_mag_y(GtkWidget *spin, gpointer data)
{
    struct gtk_graph *g = (struct gtk_graph * )data;

    g->magnify.offset.y = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin));
}

static void callback_mag_zoom(GtkWidget *spin, gpointer data)
{
    struct gtk_graph  *g = (struct gtk_graph * )data;
    double         value;
    int            direction;
    double        *zoom_this, *zoom_other;
    GtkSpinButton *widget_this, *widget_other;
    double         old_this;

    if (g->magnify.flags & MAGZOOMS_IGNORE) {
        printf("refusing callback for %s zoom widget.\n",
               ((GtkSpinButton * )spin == g->magnify.widget.h_zoom) ? "horizontal" : "vertical");
        g->magnify.flags &= ~MAGZOOMS_IGNORE;
        return;
    }
    direction = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(spin), "direction"));
    value     = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));

    if (direction) {
        zoom_this    = &g->magnify.zoom.y;
        zoom_other   = &g->magnify.zoom.x;
        widget_this  =  g->magnify.widget.v_zoom;
        widget_other =  g->magnify.widget.h_zoom;
    } else {
        zoom_this    = &g->magnify.zoom.x;
        zoom_other   = &g->magnify.zoom.y;
        widget_this  =  g->magnify.widget.h_zoom;
        widget_other =  g->magnify.widget.v_zoom;
    }

    old_this   = *zoom_this;
    *zoom_this = value;
    if (g->magnify.flags & MAGZOOMS_SAME) {
        *zoom_other = value;
        /* g->magnify.flags |= MAGZOOMS_IGNORE; */
        gtk_spin_button_set_value(widget_other, (gfloat) *zoom_other);
    } else if (g->magnify.flags & MAGZOOMS_SAME_RATIO) {
        double old_other = *zoom_other;
        *zoom_other *= value / old_this;
        if (*zoom_other < 1.0) {
            *zoom_other = 1.0;
            *zoom_this  = old_this * 1.0 / old_other;
            /* g->magnify.flags |= MAGZOOMS_IGNORE; */
            gtk_spin_button_set_value(widget_this, (gfloat) *zoom_this);
        } else if (*zoom_other > 25.0) {
            *zoom_other = 25.0;
            *zoom_this  = old_this * 25.0 / old_other;
            /* g->magnify.flags |= MAGZOOMS_IGNORE; */
            gtk_spin_button_set_value(widget_this, (gfloat) *zoom_this);
        }
        /* g->magnify.flags |= MAGZOOMS_IGNORE; */
        gtk_spin_button_set_value(widget_other, (gfloat) *zoom_other);
    }
}

static void callback_mag_flags(GtkWidget *toggle, gpointer data)
{
    struct gtk_graph *g = (struct gtk_graph * )data;
    int flag = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(toggle), "flag"));

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle)))
        g->magnify.flags |= flag;
    else
        g->magnify.flags &= ~flag;
}

static GtkWidget *control_panel_create_zoomlock_group(struct gtk_graph *g)
{
    GtkWidget *zoom_lock_h, *zoom_lock_v, *zoom_lock_none, *zoom_lock_box;
    GtkWidget *zoom_lock_frame;

    zoom_lock_none = gtk_radio_button_new_with_label(NULL, "none");
    zoom_lock_h    = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(zoom_lock_none)),
        "horizontal");
    zoom_lock_v    = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(zoom_lock_none)),
        "vertical");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(zoom_lock_none), TRUE);
    zoom_lock_box  = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0, FALSE);
    gtk_box_pack_start(GTK_BOX(zoom_lock_box), zoom_lock_none, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(zoom_lock_box), zoom_lock_h,    TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(zoom_lock_box), zoom_lock_v,    TRUE, TRUE, 0);
    zoom_lock_frame = gtk_frame_new("Zoom lock:");
    gtk_container_add(GTK_CONTAINER(zoom_lock_frame), zoom_lock_box);

    g_signal_connect(zoom_lock_h, "toggled", G_CALLBACK(callback_zoomlock_h), g);
    g_signal_connect(zoom_lock_v, "toggled", G_CALLBACK(callback_zoomlock_v), g);

    return zoom_lock_frame;
}

static void callback_zoomlock_h(GtkWidget *toggle, gpointer data)
{
    struct gtk_graph *g = (struct gtk_graph * )data;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle)))
        g->zoom.flags |= ZOOM_HLOCK;
    else
        g->zoom.flags &= ~ZOOM_HLOCK;
}

static void callback_zoomlock_v(GtkWidget *toggle, gpointer data)
{
    struct gtk_graph *g = (struct gtk_graph * )data;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle)))
        g->zoom.flags |= ZOOM_VLOCK;
    else
        g->zoom.flags &= ~ZOOM_VLOCK;
}

static GtkWidget *control_panel_create_cross_group(struct gtk_graph *g)
{
    GtkWidget *on, *off, *box, *frame, *vbox, *label;

    label = gtk_label_new("Crosshairs:");
    off   = gtk_radio_button_new_with_label(NULL, "off");
    on    = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(off)), "on");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(off), TRUE);
    box = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0, FALSE);
    gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box), off,   FALSE, FALSE, 10);
    gtk_box_pack_start(GTK_BOX(box), on,    FALSE, FALSE, 0);
    vbox  = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 0, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), box,  FALSE, FALSE, 15);
    /* frame = gtk_frame_new("Cross:"); */
    frame = gtk_frame_new(NULL);
    gtk_container_add(GTK_CONTAINER(frame), vbox);

    g_signal_connect(on, "toggled", G_CALLBACK(callback_cross_on_off), g);

    g->cross.on_toggle  = (GtkToggleButton * )on;
    g->cross.off_toggle = (GtkToggleButton * )off;

    return frame;
}
static void callback_cross_on_off(GtkWidget *toggle, gpointer data)
{
    struct gtk_graph *g = (struct gtk_graph * )data;

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle))) {
        int x, y;
        g->cross.draw = TRUE;
        get_mouse_position(g->drawing_area, &x, &y, 0);
        cross_draw(g, x, y);
    } else {
        g->cross.draw = FALSE;
        if (g->cross.erase_needed) {
            cross_erase(g);
        }
    }
}

static GtkWidget *control_panel_create_graph_type_group(struct gtk_graph *g)
{
    GtkWidget *graph_tseqttrace, *graph_tseqstevens;
    GtkWidget *graph_tput, *graph_rtt, *graph_sep, *graph_init, *graph_box;
    GtkWidget *graph_frame;
    GtkWidget *graph_wscale;

    graph_tput = gtk_radio_button_new_with_label(NULL, "Throughput");
    graph_tseqttrace = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(graph_tput)),
        "Time/Sequence (tcptrace-style)");
    graph_tseqstevens = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(graph_tput)),
        "Time/Sequence (Stevens'-style)");
    graph_rtt = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(graph_tput)),
        "Round-trip Time");
    graph_wscale = gtk_radio_button_new_with_label(
        gtk_radio_button_get_group(GTK_RADIO_BUTTON(graph_tput)),
        "Window Scaling");

    switch (g->tg.type) {
    case GRAPH_TSEQ_STEVENS:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(graph_tseqstevens), TRUE);
        break;
    case GRAPH_TSEQ_TCPTRACE:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(graph_tseqttrace), TRUE);
        break;
    case GRAPH_THROUGHPUT:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(graph_tput), TRUE);
        break;
    case GRAPH_RTT:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(graph_rtt), TRUE);
        break;
    case GRAPH_WSCALE:
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(graph_wscale), TRUE);
        break;
    case GRAPH_UNDEFINED:
        break;
    }
    graph_init = gtk_check_button_new_with_label("Init on change");
    graph_sep  = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    graph_box  = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 0, FALSE);
    gtk_box_pack_start(GTK_BOX(graph_box), graph_rtt,     TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(graph_box), graph_tput,    TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(graph_box), graph_tseqstevens, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(graph_box), graph_tseqttrace,  TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(graph_box), graph_wscale,      TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(graph_box), graph_sep,     TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(graph_box), graph_init,    TRUE, TRUE, 0);
    graph_frame = gtk_frame_new("Graph type:");
    gtk_container_add(GTK_CONTAINER(graph_frame), graph_box);

    g_object_set_data(G_OBJECT(graph_tseqstevens), "new-graph-type",
                      GINT_TO_POINTER(GRAPH_TSEQ_STEVENS));
    g_object_set_data(G_OBJECT(graph_tseqttrace), "new-graph-type",
                      GINT_TO_POINTER(GRAPH_TSEQ_TCPTRACE));
    g_object_set_data(G_OBJECT(graph_tput), "new-graph-type",
                      GINT_TO_POINTER(GRAPH_THROUGHPUT));
    g_object_set_data(G_OBJECT(graph_rtt), "new-graph-type",
                      GINT_TO_POINTER(GRAPH_RTT));
    g_object_set_data(G_OBJECT(graph_wscale), "new-graph-type",
                      GINT_TO_POINTER(GRAPH_WSCALE));

    g->gt.graph_wscale  = (GtkToggleButton * )graph_wscale;
    g->gt.graph_rtt     = (GtkToggleButton * )graph_rtt;
    g->gt.graph_tput    = (GtkToggleButton * )graph_tput;
    g->gt.graph_tseqstevens = (GtkToggleButton * )graph_tseqstevens;
    g->gt.graph_tseqttrace  = (GtkToggleButton * )graph_tseqttrace;

    g_signal_connect(graph_tseqttrace,  "toggled", G_CALLBACK(callback_graph_type), g);
    g_signal_connect(graph_tseqstevens, "toggled", G_CALLBACK(callback_graph_type), g);
    g_signal_connect(graph_tput,        "toggled", G_CALLBACK(callback_graph_type), g);
    g_signal_connect(graph_rtt,     "toggled", G_CALLBACK(callback_graph_type), g);
    g_signal_connect(graph_wscale,      "toggled", G_CALLBACK(callback_graph_type), g);
    g_signal_connect(graph_init,        "toggled", G_CALLBACK(callback_graph_init_on_typechg), g);

    return graph_frame;
}

static void callback_graph_type(GtkWidget *toggle, gpointer data)
{
    tcp_graph_type old_type, new_type;
    struct gtk_graph *g = (struct gtk_graph * )data;

    new_type = (tcp_graph_type)GPOINTER_TO_INT(g_object_get_data(G_OBJECT(toggle), "new-graph-type"));

    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(toggle)))
        return;

    old_type = g->tg.type;
    g->tg.type  = new_type;

    graph_element_lists_free(g);
    graph_element_lists_initialize(g);

    if ((old_type == GRAPH_THROUGHPUT) || (new_type == GRAPH_THROUGHPUT)) {
        /* throughput graph uses differently constructed segment list so we
         * need to recreate it */
        graph_segment_list_free(&g->tg);
        graph_segment_list_get(&cfile, &g->tg, TRUE);
    }

    if (g->flags & GRAPH_INIT_ON_TYPE_CHANGE) {
        g->geom.width  = g->wp.width;
        g->geom.height = g->wp.height;
        g->geom.x      = g->wp.x;
        g->geom.y      = g->wp.y;
    }
    g->x_axis->min = g->y_axis->min = 0;
    gtk_toggle_button_set_active(g->gui.time_orig_conn, TRUE);
    gtk_toggle_button_set_active(g->gui.seq_orig_isn, TRUE);
    graph_init_sequence(g);
}

static void callback_graph_init_on_typechg(GtkWidget *toggle _U_, gpointer data)
{
    ((struct gtk_graph * )data)->flags ^= GRAPH_INIT_ON_TYPE_CHANGE;
}

static struct gtk_graph *graph_new(void)
{
    struct gtk_graph *g;

    g = (struct gtk_graph * )g_malloc0(sizeof(struct gtk_graph));
    graph_element_lists_initialize(g);

    g->x_axis = (struct axis * )g_malloc0(sizeof(struct axis));
    g->y_axis = (struct axis * )g_malloc0(sizeof(struct axis));
    g->x_axis->g         = g;
    g->x_axis->is_horizontal_axis = TRUE;
    g->x_axis->s.x       = g->x_axis->s.y = 0;
    g->x_axis->s.height  = HAXIS_INIT_HEIGHT;
    g->x_axis->p.x       = VAXIS_INIT_WIDTH;
    g->x_axis->p.height  = HAXIS_INIT_HEIGHT;
    g->y_axis->g         = g;
    g->y_axis->is_horizontal_axis = FALSE;
    g->y_axis->p.x       = g->y_axis->p.y = 0;
    g->y_axis->p.width   = VAXIS_INIT_WIDTH;
    g->y_axis->s.x       = 0;
    g->y_axis->s.y       = TITLEBAR_HEIGHT;
    g->y_axis->s.width   = VAXIS_INIT_WIDTH;

    return g;
}

static void graph_initialize_values(struct gtk_graph *g)
{
    g->geom.width            = g->wp.width  = 750;
    g->geom.height           = g->wp.height = 550;
    g->geom.x                = g->wp.x = VAXIS_INIT_WIDTH;
    g->geom.y                = g->wp.y = TITLEBAR_HEIGHT;
    g->flags                 = 0;
    /* g->zoom.x = g->zoom.y = 1.0; */
    g->zoom.step_x           = g->zoom.step_y = 1.2;
    g->zoom.flags            = 0;
    g->cross.draw            = g->cross.erase_needed = FALSE;
    g->zoomrect_erase_needed = FALSE;
    g->grab.grabbed          = 0;
    g->magnify.active        = 0;
    g->magnify.offset.x      = g->magnify.offset.y = 0;
    g->magnify.width         = g->magnify.height = 250;
    g->magnify.zoom.x        = g->magnify.zoom.y = 10.0;
    g->magnify.flags         = 0;
}

static void graph_init_sequence(struct gtk_graph *g)
{
    debug(DBS_FENTRY) puts("graph_init_sequence()");

    graph_type_dependent_initialize(g);
    g->zoom.initial.x   = g->zoom.x;
    g->zoom.initial.y   = g->zoom.y;
    graph_element_lists_make(g);
    g->x_axis->s.width  = g->wp.width;
    g->x_axis->p.width  = g->x_axis->s.width + RMARGIN_WIDTH;
    g->x_axis->p.y      = TITLEBAR_HEIGHT + g->wp.height;
    g->x_axis->s.height = g->x_axis->p.height = HAXIS_INIT_HEIGHT;
    g->y_axis->s.height = g->wp.height;
    g->y_axis->p.height = g->wp.height + TITLEBAR_HEIGHT;
    graph_pixmaps_create(g);
    axis_pixmaps_create(g->y_axis);
    axis_pixmaps_create(g->x_axis);
    graph_title_pixmap_create(g);
    graph_title_pixmap_draw(g);
    graph_title_pixmap_display(g);
    graph_display(g);
    axis_display(g->y_axis);
    axis_display(g->x_axis);
}

static void graph_type_dependent_initialize(struct gtk_graph *g)
{
    switch (g->tg.type) {
    case GRAPH_TSEQ_STEVENS:
    case GRAPH_TSEQ_TCPTRACE:
        tseq_initialize(g);
        break;
    case GRAPH_THROUGHPUT:
        tput_initialize(g);
        break;
    case GRAPH_RTT:
        rtt_initialize(g);
        break;
    case GRAPH_WSCALE:
        wscale_initialize(g);
        break;
    default:
        break;
    }
}

static void graph_destroy(struct gtk_graph *g)
{
    debug(DBS_FENTRY) puts("graph_destroy()");

    axis_destroy(g->x_axis);
    axis_destroy(g->y_axis);
    /* window_destroy(g->drawing_area); */
    window_destroy(g->gui.control_panel);
    window_destroy(g->toplevel);
    /* window_destroy(g->text); */
#if GTK_CHECK_VERSION(2,22,0)
    if (g->title_surface) {
         cairo_surface_destroy(g->title_surface);
    }
    if (g->surface[0]) {
         cairo_surface_destroy(g->surface[0]);
    }
    if (g->surface[1]) {
         cairo_surface_destroy(g->surface[1]);
    }
#else
    g_object_unref(g->pixmap[0]);
    g_object_unref(g->pixmap[1]);
#endif /* GTK_CHECK_VERSION(2,22,0) */
    g_free(g->x_axis);
    g_free(g->y_axis);
    g_free((gpointer )(g->title));
    graph_segment_list_free(&g->tg);
    graph_element_lists_free(g);

    g_free(g);
}

static void graph_element_lists_initialize(struct gtk_graph *g)
{
    g->elists = (struct element_list *)g_malloc0(sizeof(struct element_list));
}

static void graph_element_lists_make(struct gtk_graph *g)
{
    debug(DBS_FENTRY) puts("graph_element_lists_make()");

    switch (g->tg.type) {
    case GRAPH_TSEQ_STEVENS:
        tseq_stevens_make_elmtlist(g);
        break;
    case GRAPH_TSEQ_TCPTRACE:
        tseq_tcptrace_make_elmtlist(g);
        break;
    case GRAPH_THROUGHPUT:
        tput_make_elmtlist(g);
        break;
    case GRAPH_RTT:
        rtt_make_elmtlist(g);
        break;
    case GRAPH_WSCALE:
        wscale_make_elmtlist(g);
        break;
    default:
        printf("graph_element_lists_make: unknown graph type: %d\n", g->tg.type);
        break;
    }
}

static void graph_element_lists_free(struct gtk_graph *g)
{
    struct element_list *list, *next_list;

    for (list=g->elists; list; list=next_list) {
        g_free(list->elements);
        next_list = list->next;
        g_free(list);
    }
    g->elists = NULL;   /* just to make debugging easier */
}

static void graph_title_pixmap_create(struct gtk_graph *g)
{
#if GTK_CHECK_VERSION(2,22,0)
    if (g->title_surface) {
        cairo_surface_destroy(g->title_surface);
        g->title_surface = NULL;
    }

    g->title_surface = gdk_window_create_similar_surface(gtk_widget_get_window(g->drawing_area),
                                                         CAIRO_CONTENT_COLOR,
                                                         g->x_axis->p.width,
                                                         g->wp.y);

#else
    if (g->title_pixmap)
        g_object_unref(g->title_pixmap);

    g->title_pixmap = gdk_pixmap_new(gtk_widget_get_window(g->drawing_area),
                                     g->x_axis->p.width, g->wp.y, -1);
#endif
}

static void graph_title_pixmap_draw(struct gtk_graph *g)
{
    int      i;
    cairo_t *cr;

#if GTK_CHECK_VERSION(2,22,0)
    cr = cairo_create(g->title_surface);
#else
    cr = gdk_cairo_create(g->title_pixmap);
#endif
    cairo_set_source_rgb(cr, 1, 1, 1);  /* set fill color */
    cairo_rectangle(cr, 0, 0,  g->x_axis->p.width, g->wp.y);
    cairo_fill(cr);
    cairo_set_source_rgb(cr, 0, 0, 0); /* set text color */

    for (i=0; g->title[i]; i++) {
        gint w, h;
        PangoLayout *layout;
        layout = gtk_widget_create_pango_layout(g->drawing_area,
                                                g->title[i]);
        pango_layout_get_pixel_size(layout, &w, &h);
        cairo_move_to(cr, g->wp.width/2 - w/2, 20 + i*(h+3));
        pango_cairo_show_layout(cr, layout);
        g_object_unref(G_OBJECT(layout));
    }
    cairo_destroy(cr);
}

static void graph_title_pixmap_display(struct gtk_graph *g)
{
    cairo_t *cr;

    cr = gdk_cairo_create(gtk_widget_get_window(g->drawing_area));
#if GTK_CHECK_VERSION(2,22,0)
    cairo_set_source_surface(cr, g->title_surface, g->wp.x, 0);
#else
    ws_gdk_cairo_set_source_pixmap(cr, g->title_pixmap, g->wp.x, 0);
#endif
    cairo_rectangle(cr, g->wp.x, 0, g->x_axis->p.width, g->wp.y);
    cairo_fill(cr);
    cairo_destroy(cr);
}

static void graph_pixmaps_create(struct gtk_graph *g)
{
    debug(DBS_FENTRY) puts("graph_pixmaps_create()");
#if GTK_CHECK_VERSION(2,22,0)
    if (g->surface[0]) {
        cairo_surface_destroy(g->surface[0]);
        g->surface[0] = NULL;
    }

    if (g->surface[1]) {
        cairo_surface_destroy(g->surface[1]);
        g->surface[1] = NULL;
    }

    g->surface[0] = gdk_window_create_similar_surface(gtk_widget_get_window(g->drawing_area),
                                                      CAIRO_CONTENT_COLOR,
                                                      g->wp.width,
                                                      g->wp.height);

    g->surface[1] = gdk_window_create_similar_surface(gtk_widget_get_window(g->drawing_area),
                                                      CAIRO_CONTENT_COLOR,
                                                      g->wp.width,
                                                      g->wp.height);

    g->displayed = 0;
#else
    if (g->pixmap[0])
        g_object_unref(g->pixmap[0]);
    if (g->pixmap[1])
        g_object_unref(g->pixmap[1]);

    g->pixmap[0] = gdk_pixmap_new(gtk_widget_get_window(g->drawing_area),
                                  g->wp.width, g->wp.height, -1);
    g->pixmap[1] = gdk_pixmap_new(gtk_widget_get_window(g->drawing_area),
                                  g->wp.width, g->wp.height, -1);

    g->displayed = 0;
#endif /* GTK_CHECK_VERSION(2,22,0) */
}

static void graph_display(struct gtk_graph *g)
{
    set_busy_cursor(gtk_widget_get_window(g->drawing_area));
    graph_pixmap_draw(g);
    unset_busy_cursor(gtk_widget_get_window(g->drawing_area), g->cross.draw);
    graph_pixmaps_switch(g);
    graph_pixmap_display(g);
}

static void graph_pixmap_display(struct gtk_graph *g)
{
    cairo_t *cr;

    cr = gdk_cairo_create(gtk_widget_get_window(g->drawing_area));
#if GTK_CHECK_VERSION(2,22,0)
    cairo_set_source_surface(cr, g->surface[g->displayed], g->wp.x, g->wp.y);
#else
    ws_gdk_cairo_set_source_pixmap(cr, g->pixmap[g->displayed], g->wp.x, g->wp.y);
#endif /* GTK_CHECK_VERSION(2,22,0) */
    cairo_rectangle(cr, g->wp.x, g->wp.y, g->wp.width, g->wp.height);
    cairo_fill(cr);
    cairo_destroy(cr);
    if (g->cross.erase_needed) {
        cross_erase(g);
    }
}

static void graph_pixmaps_switch(struct gtk_graph *g)
{
    g->displayed = 1 ^ g->displayed;
}

static void graph_pixmap_draw(struct gtk_graph *g)
{
    struct element_list *list;
    struct element *e;
    int       not_disp;
    cairo_t  *cr;
    GdkRGBA  *current_line_color = NULL;
    GdkRGBA  *color_to_set       = NULL;
    gboolean  line_stroked       = TRUE;

    debug(DBS_FENTRY) puts("graph_pixmap_draw()");
    not_disp = 1 ^ g->displayed;

#if GTK_CHECK_VERSION(2,22,0)
    cr = cairo_create(g->surface[not_disp]);
#else
    cr = gdk_cairo_create(g->pixmap[not_disp]);
#endif /* GTK_CHECK_VERSION(2,22,0) */
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 0, 0, g->wp.width, g->wp.height);
    cairo_fill(cr);

    /* Want line width 1 for all elements */
    cairo_set_line_width(cr, 1.0);

    for (list=g->elists; list; list=list->next) {
        for (e=list->elements; e->type != ELMT_NONE; e++) {
            switch (e->type) {
            case ELMT_RECT:
                current_line_color = NULL;
                break;

            case ELMT_LINE:
                /* Work out if we need to change colour */
                if (current_line_color == e->elment_color_p) {
                    /* No change needed */
                    color_to_set = NULL;
                }
                else {
                    /* Changing colour */
                    current_line_color = color_to_set = e->elment_color_p;
                }

                /* Draw the line */
                draw_element_line(g, e, cr, color_to_set);
                line_stroked = FALSE;
                break;

            case ELMT_ELLIPSE:
                if (!line_stroked) {
                    cairo_stroke(cr);
                    line_stroked = TRUE;
                }
                draw_element_ellipse(g, e, cr);
                break;

            default:
                break;
            }
        }
    }

    /* Make sure any remaining lines get drawn */
    if (!line_stroked)
        cairo_stroke(cr);

    cairo_destroy(cr);
}

static void draw_element_line(struct gtk_graph *g, struct element *e, cairo_t *cr,
                              GdkRGBA *new_color)
{
    int xx1, xx2, yy1, yy2;

    debug(DBS_GRAPH_DRAWING) printf("line element: (%.2f,%.2f)->(%.2f,%.2f), "
                                    "seg %u ... ", e->p.line.dim.x1, e->p.line.dim.y1,
                                    e->p.line.dim.x2, e->p.line.dim.y2, e->parent->num);

    /* Set our new colour (if changed) */
    if (new_color != NULL) {
        /* First draw any previous lines with old colour */
        cairo_stroke(cr);
        gdk_cairo_set_source_rgba(cr, new_color);
    }

    xx1 = (int )lrint(e->p.line.dim.x1 + g->geom.x - g->wp.x);
    xx2 = (int )lrint(e->p.line.dim.x2 + g->geom.x - g->wp.x);
    yy1 = (int )lrint((g->geom.height-1-e->p.line.dim.y1) + g->geom.y-g->wp.y);
    yy2 = (int )lrint((g->geom.height-1-e->p.line.dim.y2) + g->geom.y-g->wp.y);

    /* If line completely out of the area, we won't show it  */
    if (((xx1 < 0) && (xx2 < 0)) || ((xx1 >= g->wp.width)  && (xx2 >= g->wp.width)) ||
        ((yy1 < 0) && (yy2 < 0)) || ((yy1 >= g->wp.height) && (yy2 >= g->wp.height))) {
        debug(DBS_GRAPH_DRAWING) printf(" refusing: (%d,%d)->(%d,%d)\n",
                                        xx1, yy1, xx2, yy2);
        return;
    }

    /* If one end of the line is out of bounds, don't worry. Cairo will
       clip the line to the outside of g->wp at the correct angle! */

    debug(DBS_GRAPH_DRAWING) printf("line: (%d,%d)->(%d,%d)\n", xx1, yy1, xx2, yy2);

    g_assert(e->elment_color_p!=NULL);

    cairo_move_to(cr, xx1+0.5, yy1+0.5);
    cairo_line_to(cr, xx2+0.5, yy2+0.5);
}

static void draw_element_ellipse(struct gtk_graph *g, struct element *e, cairo_t *cr)
{
    gdouble w = e->p.ellipse.dim.width;
    gdouble h = e->p.ellipse.dim.height;
    gdouble x = e->p.ellipse.dim.x + g->geom.x - g->wp.x;
    gdouble y = g->geom.height-1 - e->p.ellipse.dim.y + g->geom.y - g->wp.y;

    debug(DBS_GRAPH_DRAWING) printf("ellipse: (x, y) " UTF8_RIGHTWARDS_ARROW " (w, h): (%f, %f) " UTF8_RIGHTWARDS_ARROW " (%f, %f)\n", x, y, w, h);

    cairo_save(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_translate(cr, x + w / 2., y + h / 2.);
    cairo_scale(cr, w / 2., h / 2.);
    cairo_arc(cr, 0., 0., 1., 0., 2 * G_PI);
    cairo_fill(cr);
    cairo_restore(cr);
}

static void axis_pixmaps_create(struct axis *axis)
{
    debug(DBS_FENTRY) puts("axis_pixmaps_create()");
#if GTK_CHECK_VERSION(2,22,0)
    if (axis->surface[0]) {
        cairo_surface_destroy(axis->surface[0]);
        axis->surface[0] = NULL;
    }
    if (axis->surface[1]) {
        cairo_surface_destroy(axis->surface[1]);
        axis->surface[1] = NULL;
    }
    axis->surface[0] = gdk_window_create_similar_surface(gtk_widget_get_window(axis->drawing_area),
                                                         CAIRO_CONTENT_COLOR,
                                                         axis->p.width,
                                                         axis->p.height);

    axis->surface[1] = gdk_window_create_similar_surface(gtk_widget_get_window(axis->drawing_area),
                                                         CAIRO_CONTENT_COLOR,
                                                         axis->p.width,
                                                         axis->p.height);

    axis->displayed = 0;
#else
    if (axis->pixmap[0])
        g_object_unref(axis->pixmap[0]);
    if (axis->pixmap[1])
        g_object_unref(axis->pixmap[1]);

    axis->pixmap[0] = gdk_pixmap_new(gtk_widget_get_window(axis->drawing_area),
                                     axis->p.width, axis->p.height, -1);
    axis->pixmap[1] = gdk_pixmap_new(gtk_widget_get_window(axis->drawing_area),
                                     axis->p.width, axis->p.height, -1);

    axis->displayed = 0;
#endif
}

static void axis_destroy(struct axis *axis)
{
#if GTK_CHECK_VERSION(2,22,0)
    if (axis->surface[0]) {
        cairo_surface_destroy(axis->surface[0]);
        axis->surface[0] = NULL;
    }
    if (axis->surface[1]) {
        cairo_surface_destroy(axis->surface[1]);
        axis->surface[1] = NULL;
    }
#else
    g_object_unref(axis->pixmap[0]);
    g_object_unref(axis->pixmap[1]);
#endif
    g_free( (gpointer) (axis->label) );
}

static void axis_display(struct axis *axis)
{
    if (axis->is_horizontal_axis)
        h_axis_pixmap_draw(axis);
    else
        v_axis_pixmap_draw(axis);
    axis_pixmaps_switch(axis);
    axis_pixmap_display(axis);
}

static void v_axis_pixmap_draw(struct axis *axis)
{
    struct gtk_graph *g = axis->g;
    int          i;
    double       major_tick;
    int          not_disp, rdigits, offset, imin, imax;
    double       bottom, top, j, fl, corr;
    PangoLayout *layout;
    cairo_t     *cr;

    debug(DBS_FENTRY) puts("v_axis_pixmap_draw()");
    bottom  = (g->geom.height - (g->wp.height + g->wp.y + (-g->geom.y))) /
        (double )g->geom.height * g->bounds.height;
    bottom += axis->min;
    top     = (g->geom.height - (g->wp.y + (-g->geom.y))) /
        (double )g->geom.height * g->bounds.height;
    top    += axis->min;
    axis_compute_ticks(axis, bottom, top, AXIS_VERTICAL);

    j = axis->major - floor(axis->major);
    for (rdigits=0; rdigits <= 6; rdigits++) {
        j *= 10;
        if (j <= 0.000001)
            break;
        j = j - floor(j);
    }

    not_disp = 1 ^ axis->displayed;

#if GTK_CHECK_VERSION(2,22,0)
    cr = cairo_create(axis->surface[not_disp]);
#else
    cr = gdk_cairo_create(axis->pixmap[not_disp]);
#endif
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 0, 0, axis->p.width, axis->p.height);
    cairo_fill(cr);

    /* axis */
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, axis->p.width - 1.5, (axis->p.height-axis->s.height)/2.0);
    cairo_line_to(cr, axis->s.width - 1.5, axis->p.height);

    offset = g->wp.y + (-g->geom.y);
    fl = floor(axis->min / axis->major) * axis->major;
    corr = rint((axis->min - fl) * g->zoom.y);

    /* major ticks */
    major_tick = axis->major * g->zoom.y;
    imin = (int) ((g->geom.height - offset + corr - g->wp.height) / major_tick + 1);
    imax = (int) ((g->geom.height - offset + corr) / major_tick);
    for (i=imin; i <= imax; i++) {
        gint w, h;
        char desc[32];
        int y = (int) (g->geom.height - 1 - (int )lrint(i * major_tick) -
                       offset + corr + axis->s.y);

        debug(DBS_AXES_DRAWING) printf("%f @ %d\n",
                                       (i * axis->major) + fl, y);
        if ((y < 0) || (y > axis->p.height))
            continue;

        cairo_move_to(cr, axis->p.width - 15, y + 0.5);
        cairo_line_to(cr, axis->s.width - 1,  y + 0.5);

        g_snprintf(desc, sizeof(desc), "%.*f", rdigits, i*axis->major + fl);
        layout = gtk_widget_create_pango_layout(g->drawing_area, desc);
        pango_layout_get_pixel_size(layout, &w, &h);
        cairo_move_to(cr, axis->s.width-14-4-w, y - h/2);
        pango_cairo_show_layout(cr, layout);
        g_object_unref(G_OBJECT(layout));
    }
    /* minor ticks */
    if (axis->minor) {
        double minor_tick = axis->minor * g->zoom.y;
        imin = (int) ((g->geom.height - offset + corr - g->wp.height)/minor_tick + 1);
        imax = (int) ((g->geom.height - offset + corr) / minor_tick);
        for (i=imin; i <= imax; i++) {
            int y = (int) (g->geom.height-1 - (int )lrint(i*minor_tick) -
                           offset + corr + axis->s.y);

            debug(DBS_AXES_DRAWING) printf("%f @ %d\n", i*axis->minor+fl, y);
            if ((y > 0) && (y < axis->p.height)) {
                cairo_move_to(cr, axis->s.width - 8, y+0.5);
                cairo_line_to(cr, axis->s.width - 1, y+0.5);
            }
        }
    }
    for (i=0; axis->label[i]; i++) {
        gint w, h;
        layout = gtk_widget_create_pango_layout(g->drawing_area,
                                                axis->label[i]);
        pango_layout_get_pixel_size(layout, &w, &h);
        cairo_move_to(cr,(axis->p.width - w)/2, TITLEBAR_HEIGHT-10 - i*(h+3) - h);
        pango_cairo_show_layout(cr, layout);
        g_object_unref(G_OBJECT(layout));
    }
    cairo_stroke(cr);
    cairo_destroy(cr);
}

static void h_axis_pixmap_draw(struct axis *axis)
{
    struct gtk_graph *g = axis->g;
    int           i;
    double        major_tick, minor_tick;
    int           not_disp, rdigits, offset, imin, imax;
    double        left, right, j, fl, corr;
    PangoLayout  *layout;
    cairo_t      *cr;

    debug(DBS_FENTRY) puts("h_axis_pixmap_draw()");
    left   = (g->wp.x-g->geom.x) / (double )g->geom.width * g->bounds.width;
    left  += axis->min;
    right  = (g->wp.x - g->geom.x + g->wp.width) / (double )g->geom.width * g->bounds.width;
    right += axis->min;
    axis_compute_ticks(axis, left, right, AXIS_HORIZONTAL);

    j = axis->major - floor(axis->major);
    for (rdigits=0; rdigits <= 6; rdigits++) {
        j *= 10;
        if (j <= 0.000001)
            break;
        j = j - floor(j);
    }

    not_disp = 1 ^ axis->displayed;

#if GTK_CHECK_VERSION(2,22,0)
    cr = cairo_create(axis->surface[not_disp]);
#else
    cr = gdk_cairo_create(axis->pixmap[not_disp]);
#endif
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 0, 0, axis->p.width, axis->p.height);
    cairo_fill(cr);

    /* axis */
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, 0, 0.5);
    cairo_line_to(cr, axis->s.width + (axis->p.width-axis->s.width)/2.0, 0.5);

    offset = g->wp.x - g->geom.x;

    fl = floor(axis->min / axis->major) * axis->major;
    corr = rint((axis->min - fl) * g->zoom.x);

    /* major ticks */
    major_tick = axis->major*g->zoom.x;
    imin = (int) ((offset + corr) / major_tick + 1);
    imax = (int) ((offset + corr + axis->s.width) / major_tick);
    for (i=imin; i <= imax; i++) {
        char desc[32];
        int  w, h;
        int  x = (int ) (rint(i * major_tick) - offset - corr);

        /* printf("%f @ %d\n", i*axis->major + fl, x); */
        if ((x < 0) || (x > axis->s.width))
            continue;
        cairo_move_to(cr, x+0.5, 0);
        cairo_line_to(cr, x+0.5, 15);

        g_snprintf(desc, sizeof(desc), "%.*f", rdigits, i*axis->major + fl);
        layout = gtk_widget_create_pango_layout(g->drawing_area, desc);
        pango_layout_get_pixel_size(layout, &w, &h);
        cairo_move_to(cr,  x - w/2, 15+4);
        pango_cairo_show_layout(cr, layout);
        g_object_unref(G_OBJECT(layout));
    }
    if (axis->minor > 0) {
        /* minor ticks */
        minor_tick = axis->minor*g->zoom.x;
        imin = (int) ((offset + corr) / minor_tick + 1);
        imax = (int) ((offset + corr + g->wp.width) / minor_tick);
        for (i=imin; i <= imax; i++) {
            int x = (int) (rint(i * minor_tick) - offset - corr);
            if ((x > 0) && (x < axis->s.width)) {
                cairo_move_to(cr, x+0.5, 0);
                cairo_line_to(cr, x+0.5, 8);
            }
        }
    }
    for (i=0; axis->label[i]; i++) {
        gint w, h;
        layout = gtk_widget_create_pango_layout(g->drawing_area,
                                                axis->label[i]);
        pango_layout_get_pixel_size(layout, &w, &h);
        cairo_move_to(cr,  axis->s.width - w - 50, 15+h+15 + i*(h+3));
        pango_cairo_show_layout(cr, layout);
        g_object_unref(G_OBJECT(layout));
    }
    cairo_stroke(cr);
    cairo_destroy(cr);
}

static void axis_pixmaps_switch(struct axis *axis)
{
    axis->displayed = 1 ^ axis->displayed;
}

static void axis_pixmap_display(struct axis *axis)
{
    cairo_t *cr;

    cr = gdk_cairo_create(gtk_widget_get_window(axis->drawing_area));
#if GTK_CHECK_VERSION(2,22,0)
    cairo_set_source_surface(cr, axis->surface[axis->displayed], axis->p.x, axis->p.y);
#else
    ws_gdk_cairo_set_source_pixmap(cr, axis->pixmap[axis->displayed], axis->p.x, axis->p.y);
#endif
    cairo_rectangle(cr, axis->p.x, axis->p.y, axis->p.width, axis->p.height);
    cairo_fill(cr);
    cairo_destroy(cr);

}

static void axis_compute_ticks(struct axis *axis, double x0, double xmax, int dir)
{
    int    i, j, ii, jj, ms;
    double zoom, x, steps[3] = { 0.1, 0.5 };
    int    dim, check_needed, diminished;
    double majthresh[2]  = {2.0, 3.0};

    debug((DBS_FENTRY | DBS_AXES_TICKS)) puts("axis_compute_ticks()");
    debug(DBS_AXES_TICKS)
        printf("x0=%f xmax=%f dir=%s\n", x0,xmax, dir?"VERTICAL":"HORIZONTAL");

    zoom = axis_zoom_get(axis, dir);
    x = xmax-x0;
    for (i=-9; i <= 12; i++) {
        if (x / pow(10, i) < 1)
            break;
    }
    --i;
    ms = (int )(x / pow(10, i));

    if (ms > 5) {
        j = 0;
        ++i;
    } else if (ms > 2)
        j = 1;
    else
        j = 0;

    axis->major = steps[j] * pow(10, i);

    debug(DBS_AXES_TICKS) printf("zoom=%.1f, x=%f " UTF8_RIGHTWARDS_ARROW " i=%d " UTF8_RIGHTWARDS_ARROW " ms=%d " UTF8_RIGHTWARDS_ARROW " j=%d ->"
                                 " axis->major=%f\n", zoom, x, i, ms, j, axis->major);

    /* let's compute minor ticks */
    jj = j;
    ii = i;
    axis_ticks_down(&ii, &jj);
    axis->minor = steps[jj] * pow(10, ii);
    /* we don't want minors if they would be less than 10 pixels apart */
    if (axis->minor*zoom < 10) {
        debug(DBS_AXES_TICKS) printf("refusing axis->minor of %f: "
                                     "axis->minor*zoom == %f\n", axis->minor, axis->minor*zoom);
        axis->minor = 0;
    }

    check_needed = TRUE;
    diminished = FALSE;
    while (check_needed) {
        check_needed = FALSE;
        dim = get_label_dim(axis, dir, xmax);
        debug(DBS_AXES_TICKS) printf("axis->major==%.1f, axis->minor==%.1f =>"
                                     " axis->major*zoom/dim==%f, axis->minor*zoom/dim==%f\n",
                                     axis->major, axis->minor, axis->major*zoom/dim,
                                     axis->minor*zoom/dim);

        /* corrections: if majors are less than majthresh[dir] times label
         * dimension apart, we need to use bigger ones */
        if (axis->major*zoom / dim < majthresh[dir]) {
            axis_ticks_up(&ii, &jj);
            axis->minor = axis->major;
            axis_ticks_up(&i, &j);
            axis->major = steps[j] * pow(10, i);
            check_needed = TRUE;
            debug(DBS_AXES_TICKS) printf("axis->major enlarged to %.1f\n",
                                         axis->major);
        }
        /* if minor ticks are bigger than majthresh[dir] times label dimension,
         * we could  promote them to majors as well */
        if (((axis->minor * zoom / dim) > majthresh[dir]) && !diminished) {
            axis_ticks_down(&i, &j);
            axis->major = axis->minor;
            axis_ticks_down(&ii, &jj);
            axis->minor = steps[jj] * pow(10, ii);
            check_needed = TRUE;
            diminished = TRUE;

            debug(DBS_AXES_TICKS) printf("axis->minor diminished to %.1f\n",
                                         axis->minor);

            if (axis->minor*zoom < 10) {
                debug(DBS_AXES_TICKS) printf("refusing axis->minor of %f:"
                                             " axis->minor*zoom == %f\n",
                                             axis->minor, axis->minor*zoom);
                axis->minor = 0;
            }
        }
    }

    debug(DBS_AXES_TICKS) printf("corrected: axis->major == %.1f ->"
                                 " axis->minor == %.1f\n", axis->major, axis->minor);
}

static void axis_ticks_up(int *i, int *j)
{
    (*j)++;
    if (*j > 1) {
        (*i)++;
        *j = 0;
    }
}

static void axis_ticks_down(int *i, int *j)
{
    (*j)--;
    if (*j < 0) {
        (*i)--;
        *j = 1;
    }
}

static int get_label_dim(struct axis *axis, int dir, double label)
{
    double y;
    char   str[32];
    int    rdigits, dim;
    PangoLayout *layout;

     /* First, let's compute how many digits to the right of radix
     * we need to print */
    y = axis->major - floor(axis->major);
    for (rdigits=0; rdigits <= 6; rdigits++) {
        y *= 10;
        if (y <= 0.000001)
            break;
        y = y - floor(y);
    }
    g_snprintf(str, sizeof(str), "%.*f", rdigits, label);
    switch (dir) {
    case AXIS_HORIZONTAL:
        layout = gtk_widget_create_pango_layout(axis->g->drawing_area, str);
        pango_layout_get_pixel_size(layout, &dim, NULL);
        g_object_unref(G_OBJECT(layout));
        break;
    case AXIS_VERTICAL:
        layout = gtk_widget_create_pango_layout(axis->g->drawing_area,
                                                str);
        pango_layout_get_pixel_size(layout, NULL, &dim);
        g_object_unref(G_OBJECT(layout));
        break;
    default:
        puts("initialize axis: an axis must be either horizontal or vertical");
        return -1;
    }
    return dim;
}

static double axis_zoom_get(struct axis *axis, int dir)
{
    switch (dir) {
    case AXIS_HORIZONTAL:
        return axis->g->zoom.x;
    case AXIS_VERTICAL:
        return axis->g->zoom.y;
    default:
        return -1;
    }
}

static void graph_select_segment(struct gtk_graph *g, int x, int y)
{
    struct element_list *list;
    struct element *e;
    guint num = 0;

    debug(DBS_FENTRY) puts("graph_select_segment()");

    x -= g->geom.x;
    y = g->geom.height-1 - (y - g->geom.y);

    set_busy_cursor(gtk_widget_get_window(g->drawing_area));

    for (list=g->elists; list; list=list->next) {
        for (e=list->elements; e->type != ELMT_NONE; e++) {
            switch (e->type) {
            case ELMT_RECT:
                break;
            case ELMT_LINE:
                if (line_detect_collision(e, x, y)) {
                    num = e->parent->num;
                }
                break;
            case ELMT_ELLIPSE:
                if (ellipse_detect_collision(e, x, y)) {
                    num = e->parent->num;
                }
                break;
            default:
                break;
            }
        }
    }

    if (num) {
        cf_goto_frame(&cfile, num);
    }
    unset_busy_cursor(gtk_widget_get_window(g->drawing_area), g->cross.draw);
}

static int line_detect_collision(struct element *e, int x, int y)
{
    int xx1, yy1, xx2, yy2;

    if (e->p.line.dim.x1 < e->p.line.dim.x2) {
        xx1 = (int )lrint(e->p.line.dim.x1);
        xx2 = (int )lrint(e->p.line.dim.x2);
    } else {
        xx1 = (int )lrint(e->p.line.dim.x2);
        xx2 = (int )lrint(e->p.line.dim.x1);
    }
    if (e->p.line.dim.y1 < e->p.line.dim.y2) {
        yy1 = (int )lrint(e->p.line.dim.y1);
        yy2 = (int )lrint(e->p.line.dim.y2);
    } else {
        yy1 = (int )lrint(e->p.line.dim.y2);
        yy2 = (int )lrint(e->p.line.dim.y1);
    }
    /*
    printf("line: (%d,%d)->(%d,%d), clicked: (%d,%d)\n", xx1, yy1, xx2, yy2, x, y);
     */
    if (((xx1 == x) && (xx2 == x) && (yy1 <= y) && (y <= yy2)) ||
        ((yy1 == y) && (yy2 == y) && (xx1 <= x) && (x <= xx2)))
        return TRUE;
    else
        return FALSE;
}

static int ellipse_detect_collision(struct element *e, int x, int y)
{
    int xx1, yy1, xx2, yy2;

    xx1 = (int )lrint(e->p.ellipse.dim.x);
    xx2 = (int )lrint(e->p.ellipse.dim.x + e->p.ellipse.dim.width);
    yy1 = (int )lrint(e->p.ellipse.dim.y - e->p.ellipse.dim.height);
    yy2 = (int )lrint(e->p.ellipse.dim.y);
    /*
    printf("ellipse: (%d,%d)->(%d,%d), clicked: (%d,%d)\n", xx1, yy1, xx2, yy2, x, y);
     */
    if ((xx1 <= x) && (x <= xx2) && (yy1 <= y) && (y <= yy2))
        return TRUE;
    else
        return FALSE;
}

static void cross_draw(struct gtk_graph *g, int x, int y)
{
    /* Shouldn't draw twice onto the same position if haven't erased in the
       meantime! */
    if (g->cross.erase_needed && (g->cross.x == x) && (g->cross.y == y)) {
        return;
    }

    /* Draw the cross */
    if ((x >  g->wp.x + 0.5) && (x < g->wp.x+g->wp.width) &&
        (y >  g->wp.y)       && (y < g->wp.y+g->wp.height)) {

        cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(g->drawing_area));
        gdk_cairo_set_source_rgba(cr, &g->s.tseq_tcptrace.seq_color);
        cairo_set_line_width(cr, 1.0);

        /* Horizonal line */
        cairo_move_to(cr, g->wp.x, y);
        cairo_line_to(cr, g->wp.x + g->wp.width, y);

        /* Vertical line */
        cairo_move_to(cr, x, g->wp.y);
        cairo_line_to(cr, x, g->wp.y + g->wp.height);
        cairo_stroke(cr);
        cairo_destroy(cr);
    }

    g->cross.x = x;
    g->cross.y = y;
    g->cross.erase_needed = TRUE;
}

static void zoomrect_draw(struct gtk_graph *g, int x, int y)
{
    if ((x >  g->wp.x + 0.5) && (x < g->wp.x+g->wp.width) &&
        (y >  g->wp.y)       && (y < g->wp.y+g->wp.height)) {

        cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(g->drawing_area));
        gdk_cairo_set_source_rgba(cr, &g->s.tseq_tcptrace.seq_color);
        cairo_set_line_width(cr, 1.0);

        /* Do outline of rect */
        cairo_rectangle(cr, zoomrect.x, zoomrect.y, x-zoomrect.x, y-zoomrect.y);
        cairo_stroke(cr);
        cairo_destroy(cr);
    }

    g->zoomrect_erase_needed = TRUE;
}

static void zoomrect_erase(struct gtk_graph *g)
{
    /* Just redraw what is in the pixmap buffer */
    graph_pixmap_display(g);
    g->zoomrect_erase_needed = FALSE;
}

static void cross_erase(struct gtk_graph *g)
{
    int x = g->cross.x;
    int y = g->cross.y;

    g->cross.erase_needed = FALSE;

    if ((x >  g->wp.x) && (x < g->wp.x+g->wp.width) &&
        (y >= g->wp.y) && (y < g->wp.y+g->wp.height)) {

        /* Just redraw what is in the pixmap buffer */
        graph_pixmap_display(g);
    }
}

static void magnify_move(struct gtk_graph *g, int x, int y)
{
    struct ipoint pos, offsetpos;

    get_mouse_position(g->toplevel, &pos.x, &pos.y, NULL);
    g->magnify.x = pos.x + x - g->magnify.width/2;
    g->magnify.y = pos.y + y - g->magnify.height/2;
    offsetpos.x = g->magnify.x + g->magnify.offset.x;
    offsetpos.x = offsetpos.x >= 0 ? offsetpos.x : 0;
    offsetpos.y = g->magnify.y + g->magnify.offset.y;
    offsetpos.y = offsetpos.y >= 0 ? offsetpos.y : 0;
    magnify_get_geom(g, x, y);
    magnify_draw(g);
}

static void magnify_create(struct gtk_graph *g, int x, int y)
{
    struct gtk_graph  *mg;
    struct ipoint  pos, offsetpos;
    GdkEvent      *e = NULL;
    struct element_list *list, *new_list;

    mg = g->magnify.g = (struct gtk_graph * )g_malloc(sizeof(struct gtk_graph));
    memcpy((void * )mg, (void * )g, sizeof(struct gtk_graph));

    mg->toplevel     = dlg_window_new("tcp graph magnify");
    mg->drawing_area = mg->toplevel;
    gtk_window_set_default_size(GTK_WINDOW(mg->toplevel), g->magnify.width, g->magnify.height);
    gtk_widget_set_events(mg->drawing_area, GDK_EXPOSURE_MASK
            /*      | GDK_ENTER_NOTIFY_MASK */
            /*      | GDK_ALL_EVENTS_MASK   */
        );

    mg->wp.x = 0;
    mg->wp.y = 0;
    mg->wp.width  = g->magnify.width;
    mg->wp.height = g->magnify.height;
    mg->geom.width  = (int )lrint(g->geom.width * g->magnify.zoom.x);
    mg->geom.height = (int )lrint(g->geom.height * g->magnify.zoom.y);
    mg->zoom.x = (mg->geom.width - 1) / g->bounds.width;
    mg->zoom.y = (mg->geom.height- 1) / g->bounds.height;

    /* in order to keep original element lists intact we need our own */
    graph_element_lists_initialize(mg);
    list     = g->elists->next;
    new_list = mg->elists;
    for ( ; list; list=list->next) {
        new_list->next = (struct element_list * )g_malloc(sizeof(struct element_list));
        new_list = new_list->next;
        new_list->next     = NULL;
        new_list->elements = NULL;
    }
    graph_element_lists_make(mg);

    get_mouse_position(g->toplevel, &pos.x, &pos.y, NULL);
    g->magnify.x = pos.x + x - g->magnify.width/2;
    g->magnify.y = pos.y + y - g->magnify.height/2;
    offsetpos.x  = g->magnify.x + g->magnify.offset.x;
    offsetpos.x  = offsetpos.x >= 0 ? offsetpos.x : 0;
    offsetpos.y  = g->magnify.y + g->magnify.offset.y;
    offsetpos.y  = offsetpos.y >= 0 ? offsetpos.y : 0;
    gtk_window_set_position(GTK_WINDOW(mg->drawing_area), GTK_WIN_POS_NONE);
    magnify_get_geom(g, x, y);

    gtk_widget_show(mg->drawing_area);

    /* we need to wait for the first expose event before we start drawing */
    while (!gdk_events_pending());
    do {
        e = gdk_event_get();
        if (e) {
            if (e->any.type == GDK_EXPOSE) {
                gdk_event_free(e);
                break;
            }
            gdk_event_free(e);
        }
    } while (e);

#if GTK_CHECK_VERSION(2,22,0)
    mg->surface[0] = mg->surface[1] = NULL;
#else
    mg->pixmap[0] = mg->pixmap[1] = NULL;
#endif /* GTK_CHECK_VERSION(2,22,0) */
    graph_pixmaps_create(mg);
    magnify_draw(g);
    g->magnify.active = 1;
}

static void magnify_destroy(struct gtk_graph *g)
{
    struct element_list *list;
    struct gtk_graph *mg = g->magnify.g;

    window_destroy(GTK_WIDGET(mg->drawing_area));

#if GTK_CHECK_VERSION(2,22,0)
    if (mg->surface[0]) {
         cairo_surface_destroy(mg->surface[0]);
    }
    if (mg->surface[1]) {
         cairo_surface_destroy(mg->surface[1]);
    }
#else
    g_object_unref(mg->pixmap[0]);
    g_object_unref(mg->pixmap[1]);
#endif /* GTK_CHECK_VERSION(2,22,0) */
    for (list=mg->elists; list; list=list->next)
        g_free(list->elements);

    if (mg->elists) {
        while (mg->elists->next) {
            list = mg->elists->next->next;
            g_free(mg->elists->next);
            mg->elists->next = list;
        }
    }
    g_free(g->magnify.g);
    g->magnify.active = 0;
}

static void magnify_get_geom(struct gtk_graph *g, int x, int y)
{
    int posx, posy;

    get_mouse_position(g->toplevel, &posx, &posy, NULL);

    g->magnify.g->geom.x  = g->geom.x;
    g->magnify.g->geom.y  = g->geom.y;

    g->magnify.g->geom.x -= (int )lrint((g->magnify.g->geom.width - g->geom.width) *
                                       ((x-g->geom.x)/(double )g->geom.width));
    g->magnify.g->geom.y -= (int )lrint((g->magnify.g->geom.height - g->geom.height) *
                                       ((y-g->geom.y)/(double )g->geom.height));

    /* we have coords of origin of graph relative to origin of g->toplevel.
     * now we need them to relate to origin of magnify window */
    g->magnify.g->geom.x -= (g->magnify.x - posx);
    g->magnify.g->geom.y -= (g->magnify.y - posy);
}

static void magnify_draw(struct gtk_graph *g)
{
    cairo_t *cr;
    int      not_disp = 1 ^ g->magnify.g->displayed;

    graph_pixmap_draw(g->magnify.g);
    /* graph pixmap is almost ready, just add border */
#if GTK_CHECK_VERSION(2,22,0)
    cr = cairo_create(g->magnify.g->surface[not_disp]);
#else
    cr = gdk_cairo_create(g->magnify.g->pixmap[not_disp]);
#endif /* GTK_CHECK_VERSION(2,22,0) */
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, 0, 0);
    cairo_line_to(cr, g->magnify.width - 1, 0);
    cairo_stroke(cr);

    cairo_move_to(cr, g->magnify.width - 1, 0);
    cairo_line_to(cr, g->magnify.width - 1, g->magnify.height);
    cairo_stroke(cr);

    cairo_move_to(cr, 0, 0);
    cairo_line_to(cr, 0, g->magnify.height - 1);
    cairo_stroke(cr);

    cairo_move_to(cr, 0, g->magnify.height - 1);
    cairo_line_to(cr, g->magnify.width - 1, g->magnify.height - 1);
    cairo_stroke(cr);
    cairo_destroy(cr);

    graph_pixmaps_switch(g->magnify.g);
    graph_pixmap_display(g->magnify.g);

}

static gboolean configure_event(GtkWidget *widget _U_, GdkEventConfigure *event, gpointer user_data)
{
    struct gtk_graph *g = (struct gtk_graph *)user_data;
    struct {
        double x, y;
    } zoom;
    int cur_g_width, cur_g_height;
    int cur_wp_width, cur_wp_height;

    debug(DBS_FENTRY) puts("configure_event()");

    cur_wp_width        = g->wp.width;
    cur_wp_height       = g->wp.height;
    g->wp.width         = event->width - g->y_axis->p.width - RMARGIN_WIDTH;
    g->wp.height        = event->height - g->x_axis->p.height - g->wp.y;
    g->x_axis->s.width  = g->wp.width;
    g->x_axis->p.width  = g->wp.width + RMARGIN_WIDTH;
    g->y_axis->p.height = g->wp.height + g->wp.y;
    g->y_axis->s.height = g->wp.height;
    g->x_axis->p.y      = g->y_axis->p.height;
    zoom.x              = (double )g->wp.width / cur_wp_width;
    zoom.y              = (double )g->wp.height / cur_wp_height;
    cur_g_width         = g->geom.width;
    cur_g_height        = g->geom.height;
    g->geom.width       = (int )lrint(g->geom.width * zoom.x);
    g->geom.height      = (int )lrint(g->geom.height * zoom.y);
    g->zoom.x           = (double )(g->geom.width - 1) / g->bounds.width;
    g->zoom.y           = (double )(g->geom.height -1) / g->bounds.height;
    /* g->zoom.initial.x = g->zoom.x; */
    /* g->zoom.initial.y = g->zoom.y; */

    g->geom.x = (int) (g->wp.x - (double )g->geom.width/cur_g_width *
                       (g->wp.x - g->geom.x));
    g->geom.y = (int) (g->wp.y - (double )g->geom.height/cur_g_height *
                       (g->wp.y - g->geom.y));
#if 0
    printf("configure: graph: (%d,%d), (%d,%d); viewport: (%d,%d), (%d,%d);"
           " zooms: (%f,%f)\n", g->geom.x, g->geom.y, g->geom.width,
           g->geom.height, g->wp.x, g->wp.y, g->wp.width, g->wp.height,
           g->zoom.x, g->zoom.y);
#endif

    update_zoom_spins(g);
    graph_element_lists_make(g);
    graph_pixmaps_create(g);
    graph_title_pixmap_create(g);
    axis_pixmaps_create(g->y_axis);
    axis_pixmaps_create(g->x_axis);
    /* we don't do actual drawing here; we leave it to expose handler */
    graph_pixmap_draw(g);
    graph_pixmaps_switch(g);
    graph_title_pixmap_draw(g);
    h_axis_pixmap_draw(g->x_axis);
    axis_pixmaps_switch(g->x_axis);
    v_axis_pixmap_draw(g->y_axis);
    axis_pixmaps_switch(g->y_axis);
    return TRUE;
}
#if GTK_CHECK_VERSION(3,0,0)
static gboolean
draw_event(GtkWidget *widget _U_, cairo_t *cr, gpointer user_data)
{
    struct gtk_graph *g = (struct gtk_graph *)user_data;

    debug(DBS_FENTRY) puts("draw_event()");

    /* lower left corner */
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 0, g->wp.y + g->wp.height, g->y_axis->p.width, g->x_axis->p.height);
    cairo_fill(cr);

    /* right margin */
    cairo_rectangle(cr, g->wp.x + g->wp.width, g->wp.y, RMARGIN_WIDTH, g->wp.height);
    cairo_fill(cr);

    /* Should these routines be copied here, or be given the cairo_t ??  */
    graph_pixmap_display(g);
    graph_title_pixmap_display(g);
    axis_pixmap_display(g->x_axis);
    axis_pixmap_display(g->y_axis);

    return TRUE;
}
#else
static gboolean expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
    struct gtk_graph *g = (struct gtk_graph *)user_data;
    cairo_t *cr;

    debug(DBS_FENTRY) puts("expose_event()");

    if (event->count)
        return TRUE;

    /* lower left corner */
    cr = gdk_cairo_create(gtk_widget_get_window(widget));
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 0, g->wp.y + g->wp.height, g->y_axis->p.width, g->x_axis->p.height);
    cairo_fill(cr);
    cairo_destroy(cr);
    cr = NULL;

    /* right margin */
    cr = gdk_cairo_create(gtk_widget_get_window(widget));
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, g->wp.x + g->wp.width, g->wp.y, RMARGIN_WIDTH, g->wp.height);
    cairo_fill(cr);
    cairo_destroy(cr);
    cr = NULL;

    graph_pixmap_display(g);
    graph_title_pixmap_display(g);
    axis_pixmap_display(g->x_axis);
    axis_pixmap_display(g->y_axis);

    return TRUE;
}
#endif

#define ZOOM_REDRAW   1
#define ZOOM_NOREDRAW 0
static void
perform_zoom(struct gtk_graph *g, struct zoomfactor *zf,
             int origin_x, int origin_y, int redraw)
{
    int cur_width = g->geom.width, cur_height = g->geom.height;

    /* Multiply by x and y factors */
    g->geom.width  = (int )lrint(g->geom.width  * zf->x);
    g->geom.height = (int )lrint(g->geom.height * zf->y);

    /* If already fully-zoomed out, don't waste time re-drawing */
    if ((g->geom.width <= g->wp.width) &&
        (g->geom.height <= g->wp.height)) {
        return;
    }

    if (g->geom.width < g->wp.width) {
        g->geom.width = g->wp.width;
    }
    if (g->geom.height < g->wp.height) {
        g->geom.height = g->wp.height;
    }

    /* Divide to work out new zoom */
    g->zoom.x = (g->geom.width - 1) / g->bounds.width;
    g->zoom.y = (g->geom.height- 1) / g->bounds.height;

    /* Move origin to keep mouse position at centre of view */
    g->geom.x -= (int)lrint((g->geom.width - cur_width) *
                           ((origin_x - g->geom.x)/(double )cur_width));
    g->geom.y -= (int)lrint((g->geom.height - cur_height) *
                           ((origin_y - g->geom.y)/(double )cur_height));

    if (g->geom.x > g->wp.x)
        g->geom.x = g->wp.x;
    if (g->geom.y > g->wp.y)
        g->geom.y = g->wp.y;
    if (g->wp.x + g->wp.width > g->geom.x + g->geom.width)
        g->geom.x = g->wp.width + g->wp.x - g->geom.width;
    if (g->wp.y + g->wp.height > g->geom.y + g->geom.height)
        g->geom.y = g->wp.height + g->wp.y - g->geom.height;

    if (redraw == ZOOM_NOREDRAW)
        return;

    graph_element_lists_make(g);
    g->cross.erase_needed = FALSE;
    graph_display(g);
    axis_display(g->y_axis);
    axis_display(g->x_axis);
    update_zoom_spins(g);

    if (g->cross.draw) {
        g->cross.erase_needed = FALSE;
        cross_draw(g, origin_x, origin_y);
    }
}

static void
get_zoomfactor(struct gtk_graph *g, struct zoomfactor *zf, double step_x,
               double step_y)
{
    if (g->zoom.flags & ZOOM_OUT) {
        /*
         * If can't zoom out anymore don't waste time redrawing
         * the whole graph!
         */
        if ((g->geom.height <= g->wp.height) &&
            (g->geom.width  <= g->wp.width)) {
            zf->x = 1.0;
            zf->y = 1.0;
            return;
        }
        if (g->zoom.flags & ZOOM_HLOCK)
            zf->x = 1.0;
        else
            zf->x = 1 / step_x;
        if (g->zoom.flags & ZOOM_VLOCK)
            zf->y = 1.0;
        else
            zf->y = 1 / step_y;
    } else {
        if (g->zoom.flags & ZOOM_HLOCK)
            zf->x = 1.0;
        else
            zf->x = step_x;
        if (g->zoom.flags & ZOOM_VLOCK)
            zf->y = 1.0;
        else
            zf->y = step_y;
    }
}

static void
do_zoom_rectangle(struct gtk_graph *g, struct irect lcl_zoomrect)
{
    int cur_width = g->wp.width, cur_height = g->wp.height;
    struct irect geom1 = g->geom;
    struct zoomfactor factor;

    /* Left hand too much to the right */
    if (lcl_zoomrect.x > g->wp.x + g->wp.width)
        return;
    /* Right hand not far enough */
    if (lcl_zoomrect.x + lcl_zoomrect.width < g->wp.x)
        return;
    /* Left hand too much to the left */
    if (lcl_zoomrect.x < g->wp.x) {
        int dx = g->wp.x - lcl_zoomrect.x;
        lcl_zoomrect.x += dx;
        lcl_zoomrect.width -= dx;
    }
    /* Right hand too much to the right */
    if (lcl_zoomrect.x + lcl_zoomrect.width > g->wp.x + g->wp.width) {
        int dx = lcl_zoomrect.width + lcl_zoomrect.x - g->wp.x - g->wp.width;
        lcl_zoomrect.width -= dx;
    }

    /* Top too low */
    if (lcl_zoomrect.y > g->wp.y + g->wp.height)
        return;
    /* Bottom too high */
    if (lcl_zoomrect.y + lcl_zoomrect.height < g->wp.y)
        return;
    /* Top too high */
    if (lcl_zoomrect.y < g->wp.y) {
        int dy = g->wp.y - lcl_zoomrect.y;
        lcl_zoomrect.y += dy;
        lcl_zoomrect.height -= dy;
    }
    /* Bottom too low */
    if (lcl_zoomrect.y + lcl_zoomrect.height > g->wp.y + g->wp.height) {
        int dy = lcl_zoomrect.height + lcl_zoomrect.y - g->wp.y - g->wp.height;
        lcl_zoomrect.height -= dy;
    }

/*
    printf("before:\n"
           "\tgeom: (%d, %d)+(%d x %d)\n"
*/

    get_zoomfactor(g, &factor, (double)cur_width / lcl_zoomrect.width,
                   (double)cur_height / lcl_zoomrect.height);
/*
    printf("Zoomfactor: %f x %f\n", factor.x, factor.y);
*/
    perform_zoom(g, &factor,
                 lcl_zoomrect.x, lcl_zoomrect.y,
                 ZOOM_NOREDRAW);

/*
    printf("middle:\n"
           "\tgeom: (%d, %d)+(%d x %d)\n"
           "\twp: (%d, %d)+(%d x %d)\n"
           "\tzoomrect: (%d, %d)+(%d x %d)\n",
           g->geom.x, g->geom.y,
           g->geom.width, g->geom.height,
           g->wp.x, g->wp.y, g->wp.width, g->wp.height,
           lcl_zoomrect.x, lcl_zoomrect.y, lcl_zoomrect.width, lcl_zoomrect.height);
*/
    g->geom.x = (int)(geom1.x * (1 + factor.x) -
                      lcl_zoomrect.x * factor.x - (geom1.x - g->wp.x));
    g->geom.y = (int)(geom1.y * (1 + factor.y) -
                      lcl_zoomrect.y * factor.y - (geom1.y - g->wp.y));

/*
    printf("after:\n"
           "\tgeom: (%d, %d)+(%d x %d)\n"
           "\twp: (%d, %d)+(%d x %d)\n"
           "\tzoomrect: (%d, %d)+(%d x %d)\n",
           g->geom.x, g->geom.y,
           g->geom.width, g->geom.height,
           g->wp.x, g->wp.y, g->wp.width, g->wp.height,
           lcl_zoomrect.x, lcl_zoomrect.y, lcl_zoomrect.width, lcl_zoomrect.height);
*/

    graph_element_lists_make(g);
    g->cross.erase_needed = FALSE;
    graph_display(g);
    axis_display(g->y_axis);
    axis_display(g->x_axis);
    update_zoom_spins(g);
}



static void do_zoom_mouse(struct gtk_graph *g, GdkEventButton *event)
{
    struct zoomfactor factor;

    get_zoomfactor(g, &factor, g->zoom.step_x, g->zoom.step_y);
    perform_zoom(g, &factor, (int)event->x, (int)event->y, ZOOM_REDRAW);
}

static void do_zoom_keyboard(struct gtk_graph *g)
{
    int pointer_x, pointer_y;
    struct zoomfactor factor;

    get_mouse_position(g->drawing_area, &pointer_x, &pointer_y, NULL);
    get_zoomfactor(g, &factor, g->zoom.step_x, g->zoom.step_y);
    perform_zoom(g, &factor, pointer_x, pointer_y, ZOOM_REDRAW);
}

static void do_zoom_in_keyboard(struct gtk_graph *g)
{
    gtk_toggle_button_set_active(g->zoom.widget.in_toggle, TRUE);
    do_zoom_keyboard(g);
}

static void do_zoom_out_keyboard(struct gtk_graph *g)
{
    gtk_toggle_button_set_active(g->zoom.widget.out_toggle, TRUE);
    do_zoom_keyboard(g);
    gtk_toggle_button_set_active(g->zoom.widget.in_toggle, TRUE);
}

static void do_select_segment(struct gtk_graph *g)
{
    int pointer_x, pointer_y;

    get_mouse_position(g->drawing_area, &pointer_x, &pointer_y, NULL);
    graph_select_segment(g, pointer_x, pointer_y);
}

static void do_wscale_graph(struct gtk_graph *g)
{
    gtk_toggle_button_set_active(g->gt.graph_wscale, TRUE);
}

static void do_rtt_graph(struct gtk_graph *g)
{
    gtk_toggle_button_set_active(g->gt.graph_rtt, TRUE);
}

static void do_throughput_graph(struct gtk_graph *g)
{
    gtk_toggle_button_set_active(g->gt.graph_tput, TRUE);
}

static void do_ts_graph_stevens(struct gtk_graph *g)
{
    gtk_toggle_button_set_active(g->gt.graph_tseqstevens, TRUE);
}

static void do_ts_graph_tcptrace(struct gtk_graph *g)
{
    gtk_toggle_button_set_active(g->gt.graph_tseqttrace, TRUE);
}

static void do_magnify_create(struct gtk_graph *g)
{
    int pointer_x, pointer_y;

    get_mouse_position(g->drawing_area, &pointer_x, &pointer_y, NULL);
    magnify_create(g, (int )lrint(pointer_x), (int )lrint(pointer_y));
}

static void do_key_motion(struct gtk_graph *g)
{
    if (g->geom.x > g->wp.x)
        g->geom.x = g->wp.x;
    if (g->geom.y > g->wp.y)
        g->geom.y = g->wp.y;
    if (g->wp.x + g->wp.width > g->geom.x + g->geom.width)
        g->geom.x = g->wp.width + g->wp.x - g->geom.width;
    if (g->wp.y + g->wp.height > g->geom.y + g->geom.height)
        g->geom.y = g->wp.height + g->wp.y - g->geom.height;
    g->cross.erase_needed = FALSE;
    graph_display(g);
    axis_display(g->y_axis);
    axis_display(g->x_axis);
    if (g->cross.draw) {
        int pointer_x, pointer_y;

        get_mouse_position(g->drawing_area, &pointer_x, &pointer_y, NULL);
        g->cross.erase_needed = FALSE;
        cross_draw(g, pointer_x, pointer_y);
    }
}

static void do_key_motion_up(struct gtk_graph *g, int step)
{
    g->geom.y += step;
    do_key_motion(g);
}

static void do_key_motion_down(struct gtk_graph *g, int step)
{
    g->geom.y -= step;
    do_key_motion(g);
}

static void do_key_motion_left(struct gtk_graph *g, int step)
{
    g->geom.x += step;
    do_key_motion(g);
}

static void do_key_motion_right(struct gtk_graph *g, int step)
{
    g->geom.x -= step;
    do_key_motion(g);
}

static gboolean button_press_event(GtkWidget *widget _U_, GdkEventButton *event, gpointer user_data)
{
    struct gtk_graph *g = (struct gtk_graph *)user_data;

    debug(DBS_FENTRY) puts("button_press_event()");

    if (event->button == MOUSE_BUTTON_RIGHT) {
        if (event->state & GDK_CONTROL_MASK) {
            magnify_create(g, (int )lrint(event->x), (int )lrint(event->y));
        } else {
            g->grab.x = (int )lrint(event->x) - g->geom.x;
            g->grab.y = (int )lrint(event->y) - g->geom.y;
            g->grab.grabbed = TRUE;
        }
#ifdef ORIGINAL_WIN32_BUTTONS
        /* Windows mouse control:        */
        /* [<ctrl>-left] - select packet */
        /* [left] - zoom in              */
        /* [<shift>-left] - zoom out     */
    } else if (event->button == MOUSE_BUTTON_LEFT) {
        if (event->state & GDK_CONTROL_MASK) {
            graph_select_segment(g, (int)event->x, (int)event->y);
        } else {
#else /* !ORIGINAL_WIN32_BUTTONS */
    } else if (event->button == MOUSE_BUTTON_MIDDLE) {
#endif
        /* Shift means we should zoom out */
        if (event->state & GDK_SHIFT_MASK) {
            gtk_toggle_button_set_active(g->zoom.widget.out_toggle, TRUE);
        }
        else {
            gtk_toggle_button_set_active(g->zoom.widget.in_toggle, TRUE);
        }
        do_zoom_mouse(g, event);
#ifndef ORIGINAL_WIN32_BUTTONS
    } else if (event->button == MOUSE_BUTTON_LEFT) {
        /* See if we're on an element that links to a frame */
        graph_select_segment(g, (int )event->x, (int )event->y);

        /* Record the origin of the zoom rectangle */
        zoomrect.x = (int)event->x;
        zoomrect.y = (int)event->y;
#else /* ORIGINAL_WIN32_BUTTONS*/
        }
#endif
    }
    return TRUE;
}

static gboolean motion_notify_event(GtkWidget *widget _U_, GdkEventMotion *event, gpointer user_data)
{
    struct gtk_graph *g = (struct gtk_graph *)user_data;
    int x, y;
    GdkModifierType state;

    /* debug(DBS_FENTRY) puts("motion_notify_event()"); */

    if (event->is_hint)
        get_mouse_position(g->drawing_area, &x, &y, &state);
    else {
        x = (int) event->x;
        y = (int) event->y;
        state = (GdkModifierType)event->state;
    }

    /* Testing just (state & GDK_BUTTON1_MASK) is not enough since when button1
     * is pressed while pointer is in motion, we will receive one more motion
     * notify *before* we get the button press. This last motion notify works
     * with stale grab coordinates */
    if (state & GDK_BUTTON3_MASK) {
        if (g->grab.grabbed) {
            g->geom.x = x-g->grab.x;
            g->geom.y = y-g->grab.y;

            if (g->geom.x > g->wp.x)
                g->geom.x = g->wp.x;
            if (g->geom.y > g->wp.y)
                g->geom.y = g->wp.y;
            if (g->wp.x + g->wp.width > g->geom.x + g->geom.width)
                g->geom.x = g->wp.width + g->wp.x - g->geom.width;
            if (g->wp.y + g->wp.height > g->geom.y + g->geom.height)
                g->geom.y = g->wp.height + g->wp.y - g->geom.height;
            g->cross.erase_needed = FALSE;
            graph_display(g);
            axis_display(g->y_axis);
            axis_display(g->x_axis);
            if (g->cross.draw) {
                cross_draw(g, x, y);
            }
        } else if (g->magnify.active)
            magnify_move(g, x, y);
    } else if (state & GDK_BUTTON1_MASK) {

        /* TODO: not sure we really want to jump to frames unless we release? */
        graph_select_segment(g, x, y);

        /* Update cross if necessary */
        if (g->cross.erase_needed)
            cross_erase(g);
        if (g->cross.draw)
            cross_draw(g, x, y);

        /* Draw bounded box for zoomrect being chosen! */
        if (g->zoomrect_erase_needed) {
            zoomrect_erase(g);
        }
        zoomrect_draw(g, x, y);
    } else {
        if (g->cross.erase_needed)
            cross_erase(g);
        if (g->cross.draw) {
            cross_draw(g, x, y);
        }
    }

    return TRUE;
}

static gboolean button_release_event(GtkWidget *widget _U_, GdkEventButton *event, gpointer user_data)
{
    struct gtk_graph *g = (struct gtk_graph *)user_data;

    debug(DBS_FENTRY) puts("button_release_event()");

    if (event->button == MOUSE_BUTTON_RIGHT)
        g->grab.grabbed = FALSE;

    if (event->button == MOUSE_BUTTON_LEFT) {
        int xx1 = zoomrect.x;
        int xx2 = (int)event->x;
        int yy1 = zoomrect.y;
        int yy2 = (int)event->y;
        zoomrect.x      = MIN(xx1, xx2);
        zoomrect.width  = abs(xx1 - xx2);
        zoomrect.y      = MIN(yy1, yy2);
        zoomrect.height = abs(yy1 - yy2);

        /* Finish selecting a region to zoom in on.
           Take care not to choose a too-small area (by accident?) */
        if ((zoomrect.width > 3) && (zoomrect.height > 3)) {
            int oldflags = g->zoom.flags;

            debug(DBS_GRAPH_DRAWING) printf("Zoom in from (%d, %d) - (%d, %d)\n",
                                            zoomrect.x, zoomrect.y,
                                            zoomrect.width, zoomrect.height);

            g->zoom.flags &= ~ZOOM_OUT;
            do_zoom_rectangle(g, zoomrect);
            g->zoom.flags = oldflags;
        }
    }

    if (g->magnify.active)
        magnify_destroy(g);
    return TRUE;
}

static gboolean key_press_event(GtkWidget *widget _U_, GdkEventKey *event, gpointer user_data)
{
    struct gtk_graph *g = (struct gtk_graph *)user_data;
    int step;

    debug(DBS_FENTRY) puts("key_press_event()");

    if ((event->state & GDK_CONTROL_MASK) && (event->state & GDK_SHIFT_MASK))
        step = 0;
    else if (event->state & GDK_CONTROL_MASK)
        step = 1;
    else if (event->state & GDK_SHIFT_MASK)
        step = 10;
    else
        step = 100;

    switch (event->keyval) {
    case ' ':
        toggle_crosshairs(g);
        break;
    case 't':
        toggle_time_origin(g);
        break;
    case 's':
        toggle_seq_origin(g);
        break;
    case 'r':
    case GDK_Home:
        restore_initial_graph_view(g);
        break;
    case 'i':
    case '+':
        do_zoom_in_keyboard(g);
        break;
    case 'o':
    case '-':
        do_zoom_out_keyboard(g);
        break;
    case 'm':
        do_magnify_create(g);
        break;
    case 'g':
        do_select_segment(g);
        break;
    case '1':
        do_rtt_graph(g);
        break;
    case '2':
        do_throughput_graph(g);
        break;
    case '3':
        do_ts_graph_stevens(g);
        break;
    case '4':
        do_ts_graph_tcptrace(g);
        break;
    case '5':
        do_wscale_graph(g);
        break;
    case GDK_Left:
        do_key_motion_left(g, step);
        break;
    case GDK_Up:
        do_key_motion_up(g, step);
        break;
    case GDK_Right:
        do_key_motion_right(g, step);
        break;
    case GDK_Down:
        do_key_motion_down(g, step);
        break;
    case GDK_F1:
        callback_create_help(NULL, NULL);
        break;
    default:
        break;
    }

    return TRUE;
}

static gboolean key_release_event(GtkWidget *widget _U_, GdkEventKey *event, gpointer user_data)
{
    struct gtk_graph *g = (struct gtk_graph *)user_data;

    debug(DBS_FENTRY) puts("key_release_event()");

    if ((event->keyval == GDK_Shift_L) || (event->keyval == GDK_ISO_Prev_Group)) {
        /* g->zoom.flags &= ~ZOOM_OUT; */
        gtk_toggle_button_set_active(g->zoom.widget.in_toggle, TRUE);
    }
    return TRUE;
}


static gboolean leave_notify_event(GtkWidget *widget _U_, GdkEventCrossing *event _U_, gpointer user_data)
{
    struct gtk_graph *g = (struct gtk_graph *)user_data;

    if (g->cross.erase_needed)
        cross_erase(g);

    return TRUE;
}

static gboolean enter_notify_event(GtkWidget *widget, GdkEventCrossing *event _U_, gpointer user_data)
{
    struct gtk_graph *g = (struct gtk_graph *)user_data;

    graph_pixmap_display(g);
    if (g->cross.draw) {
        int x, y;
        get_mouse_position(widget, &x, &y, NULL);
        cross_draw(g, x, y);
    }
    return TRUE;
}

static void toggle_crosshairs(struct gtk_graph *g)
{
    g->cross.draw ^= 1;
    if (g->cross.draw) {
        int x, y;
        get_mouse_position(g->drawing_area, &x, &y, NULL);
        cross_draw(g, x, y);
    } else if (g->cross.erase_needed) {
        cross_erase(g);
    }
    /* toggle buttons emit their "toggled" signals so don't bother doing
     * any real work here, it will be done in signal handlers */
    if (g->cross.draw)
        gtk_toggle_button_set_active(g->cross.on_toggle, TRUE);
    else
        gtk_toggle_button_set_active(g->cross.off_toggle, TRUE);
}

static void toggle_time_origin(struct gtk_graph *g)
{
    switch (g->tg.type) {
    case GRAPH_TSEQ_STEVENS:
        tseq_stevens_toggle_time_origin(g);
        break;
    case GRAPH_TSEQ_TCPTRACE:
        tseq_tcptrace_toggle_time_origin(g);
        break;
    case GRAPH_THROUGHPUT:
        tput_toggle_time_origin(g);
        break;
    default:
        break;
    }
    axis_display(g->x_axis);
}

static void toggle_seq_origin(struct gtk_graph *g)
{
    switch (g->tg.type) {
    case GRAPH_TSEQ_STEVENS:
        tseq_stevens_toggle_seq_origin(g);
        axis_display(g->y_axis);
        break;
    case GRAPH_TSEQ_TCPTRACE:
        tseq_tcptrace_toggle_seq_origin(g);
        axis_display(g->y_axis);
        break;
    case GRAPH_RTT:
        rtt_toggle_seq_origin(g);
        axis_display(g->x_axis);
        break;
    default:
        break;
    }
}

static void restore_initial_graph_view(struct gtk_graph *g)
{
    g->geom.width  = g->wp.width;
    g->geom.height = g->wp.height;
    g->geom.x = g->wp.x;
    g->geom.y = g->wp.y;
    graph_init_sequence(g);

    if (g->cross.draw) {
        g->cross.erase_needed = FALSE;
    }
}

/*
 * Stevens-style time-sequence grapH
 */

static void tseq_stevens_read_config(struct gtk_graph *g)
{
    debug(DBS_FENTRY) puts("tseq_stevens_read_config()");

    g->s.tseq_stevens.seq_width  = 4;
    g->s.tseq_stevens.seq_height = 4;
    g->s.tseq_stevens.flags      = 0;

    g->title = (const char ** )g_malloc(2 * sizeof(char *));
    g->title[0] = "Time/Sequence Graph (Stevens)";
    g->title[1] = NULL;
    g->y_axis->label    = (const char ** )g_malloc(3 * sizeof(char * ));
    g->y_axis->label[0] = "number[B]";
    g->y_axis->label[1] = "Sequence";
    g->y_axis->label[2] = NULL;
    g->x_axis->label    = (const char ** )g_malloc(2 * sizeof(char * ));
    g->x_axis->label[0] = "Time[s]";
    g->x_axis->label[1] = NULL;
}

/* Used by both 'stevens' and 'tcptrace' */
static void tseq_initialize(struct gtk_graph *g)
{
    debug(DBS_FENTRY) puts("tseq_initialize()");
    tseq_get_bounds(g);

    g->x_axis->min = 0;
    g->y_axis->min = 0;

    switch (g->tg.type) {
    case GRAPH_TSEQ_STEVENS:
        tseq_stevens_read_config(g);
        break;
    case GRAPH_TSEQ_TCPTRACE:
        tseq_tcptrace_read_config(g);
        break;
    default:
        break;
    }
}


/* Determine "bounds"
 *  Essentially: look for lowest/highest time and seq in the list of segments
 *  Note that for tcptrace the "(ack + window) sequence number" would normally be expected
 *   to be the upper bound; However, just to be safe, include the data seg sequence numbers
 *   in the comparison for tcptrace
 *   (e.g. to handle the case of only data segments).
 */

/* ToDo: worry about handling cases such as trying to plot seq of just 1 frame  */

static void tseq_get_bounds(struct gtk_graph *g)
{
    struct segment *tmp;
    double   tim;
    gboolean data_frame_seen = FALSE;
    double   data_tim_low    = 0;
    double   data_tim_high   = 0;
    guint32  data_seq_cur;
    guint32  data_seq_nxt;
    guint32  data_seq_low    = 0;
    guint32  data_seq_high   = 0;
    gboolean ack_frame_seen  = FALSE;
    double   ack_tim_low     = 0;
    double   ack_tim_high    = 0;
    guint32  ack_seq_cur;
    guint32  ack_seq_low     = 0;
    guint32  win_seq_cur;
    guint32  win_seq_high    = 0;

    /* go thru all segments to determine "bounds" */
    for (tmp=g->tg.segments; tmp; tmp=tmp->next) {
        if (compare_headers(&g->tg.src_address, &g->tg.dst_address,
                            g->tg.src_port, g->tg.dst_port,
                            &tmp->ip_src, &tmp->ip_dst,
                            tmp->th_sport, tmp->th_dport,
                            COMPARE_CURR_DIR)) {

            /* "data" seg */
            tim = tmp->rel_secs + tmp->rel_usecs / 1000000.0;
            data_seq_cur = tmp->th_seq;
            data_seq_nxt = data_seq_cur + tmp->th_seglen;
            if (! data_frame_seen) {
                data_tim_low    = data_tim_high = tim;
                data_seq_low    = data_seq_cur;
                data_seq_high   = data_seq_nxt;
                data_frame_seen = TRUE;
            }
            if (tim          < data_tim_low)  data_tim_low  = tim;
            if (tim          > data_tim_high) data_tim_high = tim;
            if (data_seq_cur < data_seq_low)  data_seq_low  = data_seq_cur;
            if (data_seq_nxt > data_seq_high) data_seq_high = data_seq_nxt;
        }
        else { /* ack seg */
            /* skip ack processing if no ACK (e.g. in RST) */
            if (TCP_ACK(tmp->th_flags)) {
                tim = tmp->rel_secs + tmp->rel_usecs / 1000000.0;
                ack_seq_cur = tmp->th_ack;
                win_seq_cur = ack_seq_cur + tmp->th_win;
                if (! ack_frame_seen) {
                    ack_tim_low    = ack_tim_high = tim;
                    ack_seq_low    = ack_seq_cur;
                    win_seq_high   = win_seq_cur;
                    ack_frame_seen = TRUE;
                }
                if (tim         < ack_tim_low)  ack_tim_low  = tim;
                if (tim         > ack_tim_high) ack_tim_high = tim;
                if (ack_seq_cur < ack_seq_low)  ack_seq_low  = ack_seq_cur;
                if (win_seq_cur > win_seq_high) win_seq_high = win_seq_cur;
            }
        }
    }

    /* if 'stevens':  use only data segments to determine bounds         */
    /* if 'tcptrace': use both data and ack segments to determine bounds */
    switch (g->tg.type) {
    case GRAPH_TSEQ_STEVENS:
        g->bounds.x0     = data_tim_low;
        g->bounds.width  = data_tim_high - data_tim_low;
        g->bounds.y0     = data_seq_low;
        g->bounds.height = data_seq_high - data_seq_low;
        break;
    case GRAPH_TSEQ_TCPTRACE:
        /* If (ack_frame_seen == false) -> use 'data' segments.
         * Else If (data_frame_seen == false) -> use 'ack' segments.
         * Else -> use both data and ack segments.
         */
        g->bounds.x0     = (((data_tim_low <= ack_tim_low)   && data_frame_seen) || (! ack_frame_seen))
                                    ? data_tim_low  : ack_tim_low;
        g->bounds.width  = ((((data_tim_high >= ack_tim_high) && data_frame_seen) || (! ack_frame_seen))
                                    ? data_tim_high : ack_tim_high) - g->bounds.x0;
        g->bounds.y0     = (((data_seq_low <= ack_seq_low)   && data_frame_seen) || (! ack_frame_seen))
                                    ? data_seq_low  : ack_seq_low;
        g->bounds.height = ((((data_seq_high >= win_seq_high) && data_frame_seen) || (! ack_frame_seen))
                                    ? data_seq_high : win_seq_high) - g->bounds.y0;
        break;
    default:
        break;
    }

    g->zoom.x = (g->geom.width - 1) / g->bounds.width;
    g->zoom.y = (g->geom.height -1) / g->bounds.height;
}


static void tseq_stevens_make_elmtlist(struct gtk_graph *g)
{
    struct segment *tmp;
    struct element *elements, *e;
    double  xx0      = g->bounds.x0, yy0 = g->bounds.y0;
    guint32 seq_base = (guint32) yy0;
    guint32 seq_cur;

    debug(DBS_FENTRY) puts("tseq_stevens_make_elmtlist()");
    if (g->elists->elements == NULL) {
        int n = 1 + get_num_dsegs(&g->tg);
        e = elements = (struct element * )g_malloc(n*sizeof(struct element));
    } else
        e = elements = g->elists->elements;

    for (tmp = g->tg.segments; tmp; tmp = tmp->next) {
        double secs, seqno;

        if (!compare_headers(&g->tg.src_address, &g->tg.dst_address,
                             g->tg.src_port, g->tg.dst_port,
                             &tmp->ip_src, &tmp->ip_dst,
                             tmp->th_sport, tmp->th_dport,
                             COMPARE_CURR_DIR)) {
            continue;
        }
        /* data seg */
        seq_cur = tmp->th_seq - seq_base;
        secs    = g->zoom.x * (tmp->rel_secs + tmp->rel_usecs / 1000000.0 - xx0);
        seqno   = g->zoom.y * seq_cur;

        e->type   = ELMT_ELLIPSE;
        e->parent = tmp;
        e->p.ellipse.dim.width  = g->s.tseq_stevens.seq_width;
        e->p.ellipse.dim.height = g->s.tseq_stevens.seq_height;
        e->p.ellipse.dim.x      = secs - g->s.tseq_stevens.seq_width/2.0;
        e->p.ellipse.dim.y      = seqno + g->s.tseq_stevens.seq_height/2.0;
        e++;
    }
    e->type = ELMT_NONE;
    g->elists->elements = elements;
}

static void tseq_stevens_toggle_seq_origin(struct gtk_graph *g)
{
    g->s.tseq_stevens.flags ^= SEQ_ORIGIN;

    if ((g->s.tseq_stevens.flags & SEQ_ORIGIN) == SEQ_ORIGIN_ZERO)
        g->y_axis->min = g->bounds.y0;
    else        /* g->tseq_stevens.flags & SEQ_ORIGIN == SEQ_ORIGIN_ISN */
        g->y_axis->min = 0;
}

static void tseq_stevens_toggle_time_origin(struct gtk_graph *g)
{
    g->s.tseq_stevens.flags ^= TIME_ORIGIN;

    if ((g->s.tseq_stevens.flags & TIME_ORIGIN) == TIME_ORIGIN_CAP)
        g->x_axis->min = g->bounds.x0;
    else        /* g->tseq_stevens.flags & TIME_ORIGIN == TIME_ORIGIN_CONN */
        g->x_axis->min = 0;
}

/*
 * tcptrace-style time-sequence graph
 */

static void tseq_tcptrace_read_config(struct gtk_graph *g)
{
    /* Black */
    g->s.tseq_tcptrace.seq_color.red       = (double)0 / 65535.0;
    g->s.tseq_tcptrace.seq_color.green     = (double)0 / 65535.0;
    g->s.tseq_tcptrace.seq_color.blue      = (double)0 / 65535.0;
    g->s.tseq_tcptrace.seq_color.alpha     = 1.0;

    /* LightSlateGray */
    g->s.tseq_tcptrace.ack_color[0].red    = (double)0x7777 / 65535.0;
    g->s.tseq_tcptrace.ack_color[0].green  = (double)0x8888 / 65535.0;
    g->s.tseq_tcptrace.ack_color[0].blue   = (double)0x9999 / 65535.0;
    g->s.tseq_tcptrace.ack_color[0].alpha  = 1.0;

    /* LightGray */
    g->s.tseq_tcptrace.ack_color[1].red    = (double)0xd3d3 / 65535.0;
    g->s.tseq_tcptrace.ack_color[1].green  = (double)0xd3d3 / 65535.0;
    g->s.tseq_tcptrace.ack_color[1].blue   = (double)0xd3d3 / 65535.0;
    g->s.tseq_tcptrace.ack_color[1].alpha  = 1.0;

    /* Light blue */
    g->s.tseq_tcptrace.sack_color[0].red   = (double)0x0    / 65535.0;
    g->s.tseq_tcptrace.sack_color[0].green = (double)0x0    / 65535.0;
    g->s.tseq_tcptrace.sack_color[0].blue  = (double)0xffff / 65535.0;
    g->s.tseq_tcptrace.sack_color[0].alpha = 1.0;

    /* Darker blue */
    g->s.tseq_tcptrace.sack_color[1].red   = (double)0x0    / 65535.0;
    g->s.tseq_tcptrace.sack_color[1].green = (double)0x0    / 65535.0;
    g->s.tseq_tcptrace.sack_color[1].blue  = (double)0x9888 / 65535.0;
    g->s.tseq_tcptrace.sack_color[1].alpha = 1.0;

    g->s.tseq_tcptrace.flags = 0;

    /* Allocate first list, but not elements */
    g->elists->next = (struct element_list * )
        g_malloc(sizeof(struct element_list));
    g->elists->next->next     = NULL;
    g->elists->next->elements = NULL;

    g->title = (const char ** )g_malloc(2 * sizeof(char *));
    g->title[0] = "Time/Sequence Graph (tcptrace)";
    g->title[1] = NULL;
    g->y_axis->label = (const char ** )g_malloc(3 * sizeof(char * ));
    g->y_axis->label[0] = "number[B]";
    g->y_axis->label[1] = "Sequence";
    g->y_axis->label[2] = NULL;
    g->x_axis->label    = (const char ** )g_malloc(2 * sizeof(char * ));
    g->x_axis->label[0] = "Time[s]";
    g->x_axis->label[1] = NULL;
}

static void tseq_tcptrace_make_elmtlist(struct gtk_graph *g)
{
    struct segment *tmp;
    struct element *elements0, *e0;     /* list of elmts with prio 0 */
    struct element *elements1, *e1;     /* list of elmts with prio 1 */
    double   xx0, yy0;
    double   p_t             = 0; /* ackno, window and time of previous segment */
    double   p_ackno         = 0, p_win = 0;
    gboolean ack_seen        = FALSE;
    int      toggle          = 0;
    guint32  seq_base;
    guint32  seq_cur;
    int      num_sack_ranges = 0;

    debug(DBS_FENTRY) puts("tseq_tcptrace_make_elmtlist()");

    if (g->elists->elements == NULL ) {
        /* 3 elements per data segment */
        int n = 1 + 3*get_num_dsegs(&g->tg);
        e0 = elements0 = (struct element * )g_malloc(n * sizeof(struct element));
    } else {
        /* Existing array */
        e0 = elements0 = g->elists->elements;
    }

    if (g->elists->next->elements == NULL) {
        /* 4 elements per ACK, but only one for each SACK range */
        int n = 1 + 4*get_num_acks(&g->tg, &num_sack_ranges);
        n  += num_sack_ranges;
        e1  = elements1 = (struct element * )g_malloc(n * sizeof(struct element));
    } else {
        /* Existing array */
        e1 = elements1 = g->elists->next->elements;
    }

    xx0      = g->bounds.x0;
    yy0      = g->bounds.y0;
    seq_base = (guint32) yy0;

    for (tmp = g->tg.segments; tmp; tmp = tmp->next) {
        double secs, data;
        double x;

        secs = tmp->rel_secs + tmp->rel_usecs / 1000000.0;
        x  = secs - xx0;
        x *= g->zoom.x;
        if (compare_headers(&g->tg.src_address, &g->tg.dst_address,
                            g->tg.src_port, g->tg.dst_port,
                            &tmp->ip_src, &tmp->ip_dst,
                            tmp->th_sport, tmp->th_dport,
                            COMPARE_CURR_DIR)) {
            /* forward direction (data) -> we need seqno and amount of data */
            double yy1, yy2;

            seq_cur = tmp->th_seq - seq_base;
            if (TCP_SYN(tmp->th_flags) || TCP_FIN(tmp->th_flags))
                data = 1;
            else
                data = tmp->th_seglen;

            yy1 = g->zoom.y * (seq_cur);
            yy2 = g->zoom.y * (seq_cur + data);
            e0->type   = ELMT_LINE;
            e0->parent = tmp;
            /* Set the drawing color */
            e0->elment_color_p = &g->s.tseq_tcptrace.seq_color;
            e0->p.line.dim.x1 = e0->p.line.dim.x2 = x;
            e0->p.line.dim.y1 = yy1;
            e0->p.line.dim.y2 = yy2;
            e0++;
            e0->type   = ELMT_LINE;
            e0->parent = tmp;
            /* Set the drawing color */
            e0->elment_color_p = &g->s.tseq_tcptrace.seq_color;
            e0->p.line.dim.x1 = x - 1;
            e0->p.line.dim.x2 = x + 1;
            e0->p.line.dim.y1 = e0->p.line.dim.y2 = yy1;
            e0++;
            e0->type   = ELMT_LINE;
            e0->parent = tmp;
            /* Set the drawing color */
            e0->elment_color_p = &g->s.tseq_tcptrace.seq_color;
            e0->p.line.dim.x1 = x + 1;
            e0->p.line.dim.x2 = x - 1;
            e0->p.line.dim.y1 = e0->p.line.dim.y2 = yy2;
            e0++;
        } else {
            double ackno, win;
            if (! TCP_ACK(tmp->th_flags))
                /* SYN's and RST's do not necessarily have ACK's*/
                continue;
            /* backward direction -> we need ackno and window */
            seq_cur = tmp->th_ack - seq_base;
            ackno   = seq_cur * g->zoom.y;
            win     = tmp->th_win * g->zoom.y;

            /* ack line */
            if (ack_seen == TRUE) { /* don't plot the first ack */

                /* Horizonal: time of previous ACK to now (at new ACK) */
                e1->type   = ELMT_LINE;
                e1->parent = tmp;
                /* Set the drawing color */
                e1->elment_color_p = &g->s.tseq_tcptrace.ack_color[toggle];
                e1->p.line.dim.x1 = p_t;
                e1->p.line.dim.y1 = p_ackno;
                e1->p.line.dim.x2 = x;
                e1->p.line.dim.y2 = p_ackno;
                e1++;

                /* Vertical: from previous ACKNO to current one (at current time) */
                e1->type   = ELMT_LINE;
                e1->parent = tmp;
                /* Set the drawing color */
                e1->elment_color_p = &g->s.tseq_tcptrace.ack_color[toggle];
                e1->p.line.dim.x1 = x;
                e1->p.line.dim.y1 = p_ackno;
                e1->p.line.dim.x2 = x;
                e1->p.line.dim.y2 = ((ackno != p_ackno) || (ackno < 4)) ? ackno : ackno - 4;
                e1++;

                /* Horizontal: window line */
                e1->type   = ELMT_LINE;
                e1->parent = tmp;
                /* Set the drawing color */
                e1->elment_color_p = &g->s.tseq_tcptrace.ack_color[toggle];
                e1->p.line.dim.x1 = p_t;
                e1->p.line.dim.y1 = p_win + p_ackno;
                e1->p.line.dim.x2 = x;
                e1->p.line.dim.y2 = p_win + p_ackno;
                e1++;

                /* Vertical: old window to new window */
                e1->type   = ELMT_LINE;
                e1->parent = tmp;
                /* Set the drawing color */
                e1->elment_color_p = &g->s.tseq_tcptrace.ack_color[toggle];
                e1->p.line.dim.x1 = x;
                e1->p.line.dim.y1 = p_win + p_ackno;
                e1->p.line.dim.x2 = x;
                e1->p.line.dim.y2 = win + ackno;
                e1++;

                /* Toggle color to use for ACKs... */
                toggle = 1^toggle;
            }
            ack_seen = TRUE;
            p_ackno  = ackno;
            p_win    = win;
            p_t      = x;

            /* Now any SACK entries */
            if (tmp->num_sack_ranges) {
                int n;

                for (n=0; n < tmp->num_sack_ranges; n++) {
                    double left_edge  = (tmp->sack_left_edge[n]  - seq_base) * g->zoom.y;
                    double right_edge = (tmp->sack_right_edge[n] - seq_base) * g->zoom.y;

                    /* Vertical: just show range of SACK.
                       Have experimented with sorting ranges and showing in red regions
                       between SACKs, but when TCP is limited by option bytes and needs to
                       miss out ranges, this can be pretty confusing as we end up apparently
                       NACKing what has been received... */
                    e1->type   = ELMT_LINE;
                    e1->parent = tmp;
                    /* Set the drawing color.  First range is significant, so use
                       separate colour */
                    e1->elment_color_p = (n==0) ? &g->s.tseq_tcptrace.sack_color[0] :
                                                  &g->s.tseq_tcptrace.sack_color[1];
                    e1->p.line.dim.x1 = x;
                    e1->p.line.dim.y1 = right_edge;
                    e1->p.line.dim.x2 = x;
                    e1->p.line.dim.y2 = left_edge;
                    e1++;
                }
            }
        }
    }

    /* Terminate both lists */
    e0->type = ELMT_NONE;
    e1->type = ELMT_NONE;

    g->elists->elements = elements0;
    g->elists->next->elements = elements1;
    g->elists->next->next     = NULL;
}

static void tseq_tcptrace_toggle_seq_origin(struct gtk_graph *g)
{
    g->s.tseq_tcptrace.flags ^= SEQ_ORIGIN;

    if ((g->s.tseq_tcptrace.flags & SEQ_ORIGIN) == SEQ_ORIGIN_ZERO)
        g->y_axis->min = g->bounds.y0;
    else    /* g->tseq_stevens.flags & SEQ_ORIGIN == SEQ_ORIGIN_ISN */
        g->y_axis->min = 0;
}

static void tseq_tcptrace_toggle_time_origin(struct gtk_graph *g)
{
    g->s.tseq_tcptrace.flags ^= TIME_ORIGIN;

    if ((g->s.tseq_tcptrace.flags & TIME_ORIGIN) == TIME_ORIGIN_CAP)
        g->x_axis->min = g->bounds.x0;
    else    /* g->tseq_stevens.flags & TIME_ORIGIN == TIME_ORIGIN_CONN */
        g->x_axis->min = 0;
}

/*
 * throughput graph
 */

static void tput_make_elmtlist(struct gtk_graph *g)
{
    struct segment *tmp, *oldest;
    struct element *elements, *e;
    int    i, sum = 0;
    double dtime, tput;
    int    num_sack_ranges;

    if (g->elists->elements == NULL) {
        int n = 1 + get_num_dsegs(&g->tg) + get_num_acks(&g->tg, &num_sack_ranges);
        e = elements = (struct element * )g_malloc(n * sizeof(struct element));
    } else
        e = elements = g->elists->elements;

    for (oldest=g->tg.segments, tmp=g->tg.segments->next, i=1; tmp; tmp=tmp->next, i++) {
        double time_val = tmp->rel_secs + tmp->rel_usecs/1000000.0;
        if (i>g->s.tput.ma_size) {
            oldest  = oldest->next;
            sum    -= oldest->th_seglen;
        }
        dtime = time_val - (oldest->rel_secs + oldest->rel_usecs/1000000.0);
        sum  += tmp->th_seglen;
        tput  = sum / dtime;
        /* debug(DBS_TPUT_ELMTS) printf("tput=%f\n", tput); */

        e->type   = ELMT_ELLIPSE;
        e->parent = tmp;
        e->p.ellipse.dim.width  = g->s.tput.width;
        e->p.ellipse.dim.height = g->s.tput.height;
        e->p.ellipse.dim.x      = g->zoom.x*(time_val - g->bounds.x0) - g->s.tput.width/2.0;
        e->p.ellipse.dim.y      = g->zoom.y*tput + g->s.tput.height/2.0;
        e++;
    }
    e->type = ELMT_NONE;
    g->elists->elements = elements;
}

/* Purpose of <graph_type>_initialize functions:
 * - find maximum and minimum for both axes
 * - call setup routine for style struct */
static void tput_initialize(struct gtk_graph *g)
{
    struct segment *tmp, *oldest = g->tg.segments/*, *last*/;
    int    i, sum = oldest->th_seglen;
    double dtime, tput, tputmax = 0;
    double t, t0, tmax = 0, yy0, ymax;

    debug(DBS_FENTRY) puts("tput_initialize()");

    tput_read_config(g);

    for (tmp=g->tg.segments->next, i=1; tmp; tmp=tmp->next, i++) {
        if (i > g->s.tput.ma_size) {
            oldest  = oldest->next;
            sum    -= oldest->th_seglen;
        }
        dtime = tmp->rel_secs + tmp->rel_usecs/1000000.0 -
            (oldest->rel_secs + oldest->rel_usecs/1000000.0);
        sum  += tmp->th_seglen;
        tput  = sum / dtime;
        debug(DBS_TPUT_ELMTS) printf("tput=%f\n", tput);
        if (tput > tputmax)
            tputmax = tput;
        t = tmp->rel_secs + tmp->rel_usecs / 1000000.0;
        if (t > tmax)
            tmax = t;
    }

    t0   = g->tg.segments->rel_secs + g->tg.segments->rel_usecs / 1000000.0;
    yy0  = 0;
    ymax = tputmax;

    g->bounds.x0 = t0;
    g->bounds.y0 = yy0;
    g->bounds.width  = tmax - t0;
    g->bounds.height = ymax - yy0;
    g->zoom.x = (g->geom.width - 1) / g->bounds.width;
    g->zoom.y = (g->geom.height -1) / g->bounds.height;
}

static void tput_read_config(struct gtk_graph *g)
{
    debug(DBS_FENTRY) puts("tput_read_config()");

    g->s.tput.width  = 4;
    g->s.tput.height = 4;
    g->s.tput.ma_size  = 20;

    g->title = (const char ** )g_malloc(2 * sizeof(char *));
    g->title[0] = "Throughput (20 segment MA)";
    g->title[1] = NULL;
    g->y_axis->label    = (const char ** )g_malloc(3 * sizeof(char * ));
    g->y_axis->label[0] = "[B/s]";
    g->y_axis->label[1] = "Throughput";
    g->y_axis->label[2] = NULL;
    g->x_axis->label    = (const char ** )g_malloc(2 * sizeof(char * ));
    g->x_axis->label[0] = "Time[s]";
    g->x_axis->label[1] = NULL;
    g->s.tput.flags     = 0;
}

static void tput_toggle_time_origin(struct gtk_graph *g)
{
    g->s.tput.flags ^= TIME_ORIGIN;

    if ((g->s.tput.flags & TIME_ORIGIN) == TIME_ORIGIN_CAP)
        g->x_axis->min = g->bounds.x0;
    else    /* g->s.tput.flags & TIME_ORIGIN == TIME_ORIGIN_CONN */
        g->x_axis->min = 0;
}

/* RTT graph */

static void rtt_read_config(struct gtk_graph *g)
{
    debug(DBS_FENTRY) puts("rtt_read_config()");

    g->s.rtt.width  = 4;
    g->s.rtt.height = 4;
    g->s.rtt.flags  = 0;

    g->title = (const char ** )g_malloc(2 * sizeof(char *));
    g->title[0] = "Round Trip Time Graph";
    g->title[1] = NULL;
    g->y_axis->label    = (const char ** )g_malloc(3 * sizeof(char * ));
    g->y_axis->label[0] = "RTT [s]";
    g->y_axis->label[1] = NULL;
    g->x_axis->label    = (const char ** )g_malloc(2 * sizeof(char * ));
    g->x_axis->label[0] = "Sequence Number[B]";
    g->x_axis->label[1] = NULL;
}

static void rtt_initialize(struct gtk_graph *g)
{
    struct segment *tmp, *first = NULL;
    struct unack   *unack       = NULL, *u;
    double          rttmax      = 0;
    double          xx0, yy0, ymax;
    guint32         xmax        = 0;
    guint32         seq_base    = 0;

    debug(DBS_FENTRY) puts("rtt_initialize()");

    rtt_read_config(g);

    for (tmp=g->tg.segments; tmp; tmp=tmp->next) {
        if (compare_headers(&g->tg.src_address, &g->tg.dst_address,
                            g->tg.src_port, g->tg.dst_port,
                            &tmp->ip_src, &tmp->ip_dst,
                            tmp->th_sport, tmp->th_dport,
                            COMPARE_CURR_DIR)) {
            guint32 seqno = tmp->th_seq;

            if (!first) {
                first    = tmp;
                seq_base = seqno;
            }
            seqno -= seq_base;
            if (tmp->th_seglen && !rtt_is_retrans(unack, seqno)) {
                double time_val = tmp->rel_secs + tmp->rel_usecs / 1000000.0;
                u = rtt_get_new_unack(time_val, seqno);
                if (!u) return;
                rtt_put_unack_on_list(&unack, u);
            }

            if (seqno + tmp->th_seglen > xmax)
                xmax = seqno + tmp->th_seglen;
        } else if (first) {
            guint32       ackno    = tmp->th_ack -seq_base;
            double        time_val = tmp->rel_secs + tmp->rel_usecs / 1000000.0;
            struct unack *v;

            for (u=unack; u; u=v) {
                if (ackno > u->seqno) {
                    double rtt = time_val - u->time;
                    if (rtt > rttmax)
                        rttmax = rtt;
                    v = u->next;
                    rtt_delete_unack_from_list(&unack, u);
                } else
                    v = u->next;
            }
        }
    }

    xx0  = seq_base;
    yy0  = 0;
    ymax = rttmax;

    g->bounds.x0 = xx0;
    g->bounds.y0 = yy0;
    g->bounds.width  = xmax;
    g->bounds.height = ymax - yy0;
    g->zoom.x = g->geom.width  / g->bounds.width;
    g->zoom.y = g->geom.height / g->bounds.height;
}

static void rtt_make_elmtlist(struct gtk_graph *g)
{
    struct segment *tmp;
    struct unack   *unack    = NULL, *u;
    struct element *elements, *e;
    guint32         seq_base = (guint32) g->bounds.x0;

    debug(DBS_FENTRY) puts("rtt_make_elmtlist()");

    if (g->elists->elements == NULL) {
        int n = 1 + get_num_dsegs(&g->tg);
        e = elements = (struct element * )g_malloc(n * sizeof(struct element));
    } else {
        e = elements = g->elists->elements;
    }

    for (tmp=g->tg.segments; tmp; tmp=tmp->next) {
        if (compare_headers(&g->tg.src_address, &g->tg.dst_address,
                            g->tg.src_port, g->tg.dst_port,
                            &tmp->ip_src, &tmp->ip_dst,
                            tmp->th_sport, tmp->th_dport,
                            COMPARE_CURR_DIR)) {
            guint32 seqno = tmp->th_seq -seq_base;

            if (tmp->th_seglen && !rtt_is_retrans(unack, seqno)) {
                double time_val = tmp->rel_secs + tmp->rel_usecs / 1000000.0;
                u = rtt_get_new_unack(time_val, seqno);
                if (!u) return;
                rtt_put_unack_on_list(&unack, u);
            }
        } else {
            guint32       ackno    = tmp->th_ack -seq_base;
            double        time_val = tmp->rel_secs + tmp->rel_usecs / 1000000.0;
            struct unack *v;

            for (u=unack; u; u=v) {
                if (ackno > u->seqno) {
                    double rtt = time_val - u->time;

                    e->type   = ELMT_ELLIPSE;
                    e->parent = tmp;
                    e->p.ellipse.dim.width  = g->s.rtt.width;
                    e->p.ellipse.dim.height = g->s.rtt.height;
                    e->p.ellipse.dim.x      = g->zoom.x * u->seqno - g->s.rtt.width/2.0;
                    e->p.ellipse.dim.y      = g->zoom.y * rtt + g->s.rtt.height/2.0;
                    e++;

                    v = u->next;
                    rtt_delete_unack_from_list(&unack, u);
                } else
                    v = u->next;
            }
        }
    }
    e->type = ELMT_NONE;
    g->elists->elements = elements;
}

static void rtt_toggle_seq_origin(struct gtk_graph *g)
{
    g->s.rtt.flags ^= SEQ_ORIGIN;

    if ((g->s.rtt.flags & SEQ_ORIGIN) == SEQ_ORIGIN_ZERO)
        g->x_axis->min = g->bounds.x0;
    else
        g->x_axis->min = 0;
}

/* WSCALE Graph */

static void wscale_read_config(struct gtk_graph *g)
{
    debug(DBS_FENTRY) puts("wscale_read_config()");

    g->s.wscale.win_width  = 4;
    g->s.wscale.win_height = 4;
    g->s.wscale.flags      = 0;

    g->title = (const char ** )g_malloc(2 * sizeof(char *));
    g->title[0] = "Window Scaling Graph";
    g->title[1] = NULL;
    g->y_axis->label    = (const char ** )g_malloc(3 * sizeof(char * ));
    g->y_axis->label[0] = "[bytes]";
    g->y_axis->label[1] = "Windowsize";
    g->y_axis->label[2] = NULL;
    g->x_axis->label    = (const char ** )g_malloc(2 * sizeof(char * ));
    g->x_axis->label[0] = "Time [s]";
    g->x_axis->label[1] = NULL;
}

/*
    (1) Find maximum and minimum values for Window-Size(scaled) and seconds
    (2) call function to define window related values
*/
static void wscale_initialize(struct gtk_graph *g)
{

    struct segment *segm = NULL;
    guint32 wsize_max =  0;
    guint32 wsize_min =  0;
    gdouble sec_max   =  0.0;
    gdouble sec_base  = -1.0;

    wscale_read_config(g);

    debug(DBS_FENTRY) puts("wscale_initialize()");

    for (segm=g->tg.segments; segm; segm=segm->next) {
        if (compare_headers(&g->tg.src_address, &g->tg.dst_address,
                            g->tg.src_port, g->tg.dst_port,
                            &segm->ip_src, &segm->ip_dst,
                            segm->th_sport, segm->th_dport,
                            COMPARE_CURR_DIR))
        {
            gdouble sec   = segm->rel_secs + ( segm->rel_usecs / 1000000.0 );
            guint16 flags = segm->th_flags;
            guint32 wsize = segm->th_win;

            /* only data segments */
            if ( (flags & (TH_SYN|TH_RST)) == 0 )
                if ( wsize > wsize_max )
                    wsize_max = wsize;

            /* remind time of first probe */
            if ((sec_base < 0) && (sec > 0))
                sec_base = sec;

            if ( sec_max < sec )
                sec_max = sec;

        }

    }

    g->bounds.x0 = 0;
    g->bounds.y0 = wsize_min;
    g->bounds.width  = sec_max - sec_base + 5;
    g->bounds.height = wsize_max + 5;
    g->zoom.x = g->geom.width / g->bounds.width;
    g->zoom.y = g->geom.height / g->bounds.height;

}

/*
   (1) Fill & allocate memory for segments times elements,
*/
static void wscale_make_elmtlist(struct gtk_graph *g)
{
    struct segment *segm     = NULL;
    struct element *elements = NULL;
    struct element *e        = NULL;
    gdouble         sec_base = -1.0;

    debug(DBS_FENTRY) puts("wscale_make_elmtlist()");

    /* Allocate memory for elements if not already done */
    if (g->elists->elements == NULL)
    {
        int n = 1 + get_num_dsegs(&g->tg);
        e = elements = (struct element*)g_malloc(n*sizeof(struct element));
    }
    else
        e = elements = g->elists->elements;


    for ( segm=g->tg.segments; segm; segm=segm->next ) {
        if (compare_headers(&g->tg.src_address, &g->tg.dst_address,
                            g->tg.src_port, g->tg.dst_port,
                            &segm->ip_src, &segm->ip_dst,
                            segm->th_sport, segm->th_dport,
                            COMPARE_CURR_DIR))
        {
            gdouble sec   = segm->rel_secs + (segm->rel_usecs / 1000000.0);
            guint16 flags = segm->th_flags;
            guint32 wsize = segm->th_win;

            /* remind time of first probe */
            if ((sec_base < 0) && (sec > 0))
                sec_base = sec;

            /* only data or ack segments */
            if ( (flags & (TH_SYN|TH_RST)) == 0 )
            {
                e->type   = ELMT_ELLIPSE;
                e->parent = segm;
                e->p.ellipse.dim.width  = g->s.wscale.win_width;
                e->p.ellipse.dim.height = g->s.wscale.win_height;
                e->p.ellipse.dim.x      = g->zoom.x * (sec - sec_base) - g->s.wscale.win_width / 2.0;
                e->p.ellipse.dim.y      = g->zoom.y * wsize - g->s.wscale.win_height / 2.0;
                e++;
            }
        }
    }
    /* finished populating element list */
    e->type = ELMT_NONE;
    g->elists->elements = elements;
}

#if defined(_WIN32) && !defined(__MINGW32__) && (_MSC_VER < 1800)
/* Starting VS2103, rint already defined in math.h. No need to redefine */
/* replacement of Unix rint() for Windows */
static int rint(double x)
{
    char *buf;
    int   i, dec, sig;

    buf = _fcvt(x, 0, &dec, &sig);
    i = atoi(buf);
    if (sig == 1) {
        i = i * -1;
    }
    return(i);
}
#endif

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
