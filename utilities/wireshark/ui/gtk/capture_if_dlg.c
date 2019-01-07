/* capture_if_dlg.c
 * Routines for the capture interface dialog
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

#include <glib.h>

#ifdef HAVE_LIBPCAP

#include <string.h>

#include <epan/prefs.h>
#include <epan/to_str.h>

#include "../../capture_opts.h"
#include <capchild/capture_session.h>
#include "caputils/capture_ifinfo.h"
#include "ui/capture.h"
#include "caputils/capture-pcap-util.h"
#include "wsutil/file_util.h"
#include <wiretap/wtap.h>

#include "ui/capture_ui_utils.h"
#include "ui/capture_globals.h"
#include "ui/recent.h"
#include "ui/simple_dialog.h"

#ifdef _WIN32
#include "ui/gtk/capture_if_details_dlg_win32.h"
#include "caputils/capture-wpcap.h"
#endif

#include "ui/gtk/stock_icons.h"
#ifndef HAVE_GDK_GRESOURCE
#include "ui/gtk/pixbuf-csource.h"
#endif
#include "ui/gtk/capture_dlg.h"
#include "ui/gtk/capture_if_dlg.h"
#include "ui/gtk/gui_utils.h"
#include "ui/gtk/dlg_utils.h"
#include "ui/gtk/main.h"
#include "ui/gtk/main_toolbar.h"
#include "ui/gtk/help_dlg.h"
#include "ui/gtk/keys.h"
#include "ui/gtk/webbrowser.h"
#include "ui/gtk/main_welcome.h"

#include "ui/gtk/old-gtk-compat.h"

#ifdef HAVE_AIRPCAP
#include "../../image/toolbar/capture_airpcap_16.xpm"
#endif

#include "../../image/toolbar/modem_16.xpm"

#include "../../image/toolbar/network_virtual_16.xpm"

/* new buttons to be used instead of labels for 'Capture','Prepare',' */
/*#include "../../image/toolbar/capture_capture_16.xpm"*/
/*#include "../../image/toolbar/capture_prepare_16.xpm"*/
/*#include "../../image/toolbar/capture_details_16.xpm"*/


#ifdef HAVE_AIRPCAP
#include <caputils/airpcap.h>
#include <caputils/airpcap_loader.h>
#include "airpcap_gui_utils.h"
#include "airpcap_dlg.h"
#endif

#define CAPTURE_IF_IP_ADDR_LABEL      "capture_if_ip_addr_label"
#define CAPTURE_IF_SELECTED_IP_ADDR   "capture_if_selected_ip_addr"
#define SOFT_LABEL_LEN 50

/*
 * Keep a static pointer to the current "Capture Interfaces" window, if
 * any, so that if somebody tries to do "Capture:Start" while there's
 * already a "Capture Interfaces" window up, we just pop up the existing
 * one, rather than creating a new one.
 */
static GtkWidget       *cap_if_w;

static guint            timer_id;

static GtkWidget       *close_bt, *stop_bt, *capture_bt, *options_bt;

static GArray          *if_array;

static if_stat_cache_t *sc;
static GtkWidget       *cap_if_top_vb, *cap_if_sw;

/* the "runtime" data of one interface */
typedef struct if_dlg_data_s {
    gchar     *device;
    GtkWidget *device_lb;
    GtkWidget *descr_lb;
    GtkWidget *ip_lb;
    GtkWidget *curr_lb;
    GtkWidget *last_lb;
    GtkWidget *choose_bt;
#ifdef _WIN32
    GtkWidget *details_bt;
#endif
    gboolean   hidden;
} if_dlg_data_t;

static gboolean gbl_capture_in_progress = FALSE;

#if 0
void
add_interface(void)
{
  if_dlg_data_t data;

  data.device_lb  = NULL;
  data.descr_lb   = NULL;
  data.ip_lb      = NULL;
  data.curr_lb    = NULL;
  data.last_lb    = NULL;
  data.choose_bt  = NULL;
#ifdef _WIN32
  data.details_bt = NULL;
#endif
  data.hidden     = FALSE;
  g_array_append_val(if_array, data);
  refresh_if_window();
}
#endif

void
update_selected_interface(gchar *name)
{
  guint         i;
  interface_t   device;
  if_dlg_data_t data;

  for (i = 0; i < global_capture_opts.all_ifaces->len; i++) {
    device = g_array_index(global_capture_opts.all_ifaces, interface_t, i);
    data = g_array_index(if_array, if_dlg_data_t, i);
    if (strcmp(name, device.name) == 0) {
      gtk_toggle_button_set_active((GtkToggleButton *)data.choose_bt, device.selected);
      break;
    }
  }
}

static void
store_selected(GtkWidget *choose_bt, gpointer name)
{
  interface_t device;
  guint       i;

  for (i = 0; i < global_capture_opts.all_ifaces->len; i++) {
    device = g_array_index(global_capture_opts.all_ifaces, interface_t, i);
    if (strcmp((char *)name, device.if_info.name) == 0) {
      if (!device.locked) {
        device.selected ^= 1;
        if (device.selected) {
          global_capture_opts.num_selected++;
        } else {
          global_capture_opts.num_selected--;
        }
        global_capture_opts.all_ifaces = g_array_remove_index(global_capture_opts.all_ifaces, i);
        g_array_insert_val(global_capture_opts.all_ifaces, i, device);
        if (gtk_widget_is_focus(choose_bt) && get_welcome_window()) {
          change_interface_selection(device.name, device.selected);
        }
        if (gtk_widget_is_focus(choose_bt) && capture_dlg_window_present()) {
          enable_selected_interface(device.name, device.selected);
        }
        device.locked = FALSE;
        global_capture_opts.all_ifaces = g_array_remove_index(global_capture_opts.all_ifaces, i);
        g_array_insert_val(global_capture_opts.all_ifaces, i, device);
      }
      break;
    }
  }
  if (cap_if_w) {
    gtk_widget_set_sensitive(capture_bt, !gbl_capture_in_progress && (global_capture_opts.num_selected > 0));
  }
}

/* start capture button was pressed */
static void
capture_do_cb(GtkWidget *capture_bt_arg _U_, gpointer if_data _U_)
{
  if_dlg_data_t data;
  guint         ifs;

  for (ifs = 0; ifs < if_array->len; ifs++) {
    data = g_array_index(if_array, if_dlg_data_t, ifs);
    if (data.hidden) {
      continue;
    }
    gtk_widget_set_sensitive(data.choose_bt, FALSE);
    if_array = g_array_remove_index(if_array, ifs);
    g_array_insert_val(if_array, ifs, data);
#ifdef HAVE_AIRPCAP
    airpcap_if_active = get_airpcap_if_from_name(g_airpcap_if_list, gtk_label_get_text(GTK_LABEL(data.device_lb)));
    airpcap_if_selected = airpcap_if_active;
#endif
  }

  /* XXX - remove this? */
  if (global_capture_opts.save_file) {
    g_free(global_capture_opts.save_file);
    global_capture_opts.save_file = NULL;
  }

  if (global_capture_opts.ifaces->len > 1) {
    global_capture_opts.use_pcapng = TRUE;
  }
  /* stop capturing from all interfaces, we are going to do real work now ... */
  window_destroy(cap_if_w);

  capture_start_cb(NULL, NULL);
}


/* prepare capture button was pressed */
static void
capture_prepare_cb(GtkWidget *prepare_bt _U_, gpointer if_data _U_)
{
  /* stop capturing from all interfaces, we are going to do real work now ... */
  window_destroy(cap_if_w);
  if (global_capture_opts.ifaces->len > 1) {
    global_capture_opts.use_pcapng = TRUE;
  }
  capture_prep_cb(NULL, NULL);
}


#ifdef _WIN32
/* capture details button was pressed */
static void
capture_details_cb(GtkWidget *details_bt _U_, gpointer name)
{
  capture_if_details_open(name);
}
#endif

/* update a single interface */
static void
update_if(gchar *name, if_stat_cache_t *sc_p)
{
  struct pcap_stat  stats;
  gchar            *str;
  guint             diff, ifs, data_ifs;
  interface_t       device;
  if_dlg_data_t     data;
  gboolean          found = FALSE;


  /*
   * Note that some versions of libpcap, on some versions of UN*X,
   * pcap_stats() returns the number of packets since the last
   * pcap_stats call.
   *
   * That's a bug, and should be fixed; "pcap_stats()" is supposed
   * to work the same way on all platforms.
   */
  device.last_packets = 0;
  data.curr_lb = NULL;
  data.last_lb = NULL;
  if (sc_p) {
    for (ifs = 0, data_ifs = 0; ifs < global_capture_opts.all_ifaces->len; ifs++) {
      device = g_array_index(global_capture_opts.all_ifaces, interface_t, ifs);
      if (device.type != IF_PIPE) {
        data = g_array_index(if_array, if_dlg_data_t, data_ifs++);
        if (!device.hidden && strcmp(name, device.name) == 0) {
          found = TRUE;
          break;
        }
      }
    }
    if (found) {
      if (capture_stats(sc_p, name, &stats)) {
        if ((int)(stats.ps_recv - device.last_packets) < 0) {
          diff = 0;
        } else {
          diff = stats.ps_recv - device.last_packets;
        }
        device.last_packets = stats.ps_recv;

        str = g_strdup_printf("%u", device.last_packets);
        gtk_label_set_text(GTK_LABEL(data.curr_lb), str);
        g_free(str);
        str = g_strdup_printf("%u", diff);
        gtk_label_set_text(GTK_LABEL(data.last_lb), str);
        g_free(str);

        gtk_widget_set_sensitive(data.curr_lb, diff);
        gtk_widget_set_sensitive(data.last_lb, diff);
      } else {
        gtk_label_set_text(GTK_LABEL(data.curr_lb), "error");
        gtk_label_set_text(GTK_LABEL(data.last_lb), "error");
      }
      global_capture_opts.all_ifaces = g_array_remove_index(global_capture_opts.all_ifaces, ifs);
      g_array_insert_val(global_capture_opts.all_ifaces, ifs, device);
      if_array = g_array_remove_index(if_array, ifs);
      g_array_insert_val(if_array, ifs, data);
    }
  }
}

/* update all interfaces */
static gboolean
update_all(gpointer data)
{
  interface_t      device;
  guint            ifs;
  if_stat_cache_t *sc_p = (if_stat_cache_t *)data;

  if (!cap_if_w) {
    return FALSE;
  }

  for (ifs = 0; ifs < global_capture_opts.all_ifaces->len; ifs++) {
    device = g_array_index(global_capture_opts.all_ifaces, interface_t, ifs);
    update_if(device.name, sc_p);
  }

  return TRUE;
}

/* a live capture has started or stopped */
void
set_capture_if_dialog_for_capture_in_progress(gboolean capture_in_progress)
{
  gbl_capture_in_progress = capture_in_progress;
  if (cap_if_w) {
    gtk_widget_set_sensitive(stop_bt, capture_in_progress);
    gtk_widget_set_sensitive(capture_bt, !capture_in_progress && (global_capture_opts.num_selected > 0));
  }
}

/* a live capture is being stopped */
void
set_capture_if_dialog_for_capture_stopping(void)
{
  if (cap_if_w) {
    gtk_widget_set_sensitive(stop_bt, FALSE);
  }
}


/* the window was closed, cleanup things */
static void
capture_if_destroy_cb(GtkWidget *win _U_, gpointer user_data _U_)
{
  g_source_remove(timer_id);

  if (sc) {
    capture_stat_stop(sc);
    sc = NULL;
  }
  window_destroy(GTK_WIDGET(cap_if_w));
  /* Note that we no longer have a "Capture Options" dialog box. */
  cap_if_w      = NULL;
  cap_if_top_vb = NULL;
  cap_if_sw     = NULL;
#ifdef HAVE_AIRPCAP
  if (airpcap_if_active)
    airpcap_set_toolbar_stop_capture(airpcap_if_active);
#endif
}


/*
 * Used to retrieve the interface icon.
 */
GtkWidget * capture_get_if_icon(interface_t *device)
{
#ifdef HAVE_PCAP_REMOTE
  if (!device->local) {
    return PIXBUF_TO_WIDGET(remote_sat_pb_data, "/org/wireshark/image/toolbar/remote_sat_16.png");
  }
#endif
  if (device->display_name && strstr(device->display_name,"Wi-Fi") != NULL) {
    return PIXBUF_TO_WIDGET(network_wireless_pb_data, "/org/wireshark/image/toolbar/network_wireless_16.png");
  }
  switch (device->type) {
  case IF_DIALUP:
    return xpm_to_widget(modem_16_xpm);
  case IF_WIRELESS:
    return PIXBUF_TO_WIDGET(network_wireless_pb_data, "/org/wireshark/image/toolbar/network_wireless_16.png");
#ifdef HAVE_AIRPCAP
  case IF_AIRPCAP:
    return xpm_to_widget(capture_airpcap_16_xpm);
#endif
  case IF_BLUETOOTH:
    return PIXBUF_TO_WIDGET(network_bluetooth_pb_data, "/org/wireshark/image/toolbar/network_bluetooth_16.png");
  case IF_USB:
    return PIXBUF_TO_WIDGET(network_usb_pb_data, "/org/wireshark/image/toolbar/network_usb_16.png");
  case IF_VIRTUAL:
    return xpm_to_widget(network_virtual_16_xpm);
  case IF_WIRED:
    return PIXBUF_TO_WIDGET(network_wired_pb_data, "/org/wireshark/image/toolbar/network_wired_16.png");
#ifdef HAVE_EXTCAP
  case IF_EXTCAP:
#ifdef _WIN32
    if (strncmp(device->friendly_name, "USBPcap", 7) == 0) {
      return PIXBUF_TO_WIDGET(network_usb_pb_data, "/org/wireshark/image/toolbar/network_usb_16.png");
    }
#endif
#endif
  case IF_PIPE:
  case IF_STDIN:
    return PIXBUF_TO_WIDGET(pipe_pb_data, "/org/wireshark/image/toolbar/pipe_16.png");
  default:
    printf("unknown device type\n");
  }
  return PIXBUF_TO_WIDGET(network_wired_pb_data, "/org/wireshark/image/toolbar/network_wired_16.png");
}


static int
get_ip_addr_count(GSList *addr_list)
{
  GSList    *curr_addr;
  if_addr_t *addr;
  int        count;

  count = 0;
  for (curr_addr = addr_list; curr_addr != NULL;
       curr_addr = g_slist_next(curr_addr)) {
    addr = (if_addr_t *)curr_addr->data;
    switch (addr->ifat_type) {

    case IF_AT_IPv4:
    case IF_AT_IPv6:
      count++;
      break;

    default:
      /* In case we add non-IP addresses */
      break;
    }
  }
  return count;
}

static const gchar *
set_ip_addr_label(GSList *addr_list, GtkWidget *ip_lb, guint selected_ip_addr)
{
  GSList      *curr_addr;
  if_addr_t   *addr;
  address     addr_address;
  char       *addr_str = NULL;

  curr_addr = g_slist_nth(addr_list, selected_ip_addr);
  if (curr_addr) {
    addr = (if_addr_t *)curr_addr->data;
    switch (addr->ifat_type) {

    case IF_AT_IPv4:
      set_address(&addr_address, AT_IPv4, 4, &addr->addr.ip4_addr);
      addr_str = (char*)address_to_str(NULL, &addr_address);
      break;

    case IF_AT_IPv6:
      set_address(&addr_address, AT_IPv6, 16, addr->addr.ip6_addr);
      addr_str = (char*)address_to_str(NULL, &addr_address);
      break;

    default:
      /* Ignore non-IP addresses, in case we ever support them */
      break;
    }
  }

  if (addr_str) {
    gtk_label_set_text(GTK_LABEL(ip_lb), addr_str);
  } else {
    gtk_label_set_text(GTK_LABEL(ip_lb), "none");
  }
  wmem_free(NULL, addr_str);
  g_object_set_data(G_OBJECT(ip_lb), CAPTURE_IF_SELECTED_IP_ADDR, GINT_TO_POINTER(selected_ip_addr));

  return addr_str;
}


static gboolean
ip_label_enter_cb(GtkWidget *eb, GdkEventCrossing *event _U_, gpointer user_data _U_)
{
    gtk_widget_set_state(eb, GTK_STATE_PRELIGHT);

    return FALSE;
}


static gboolean
ip_label_leave_cb(GtkWidget *eb, GdkEvent *event _U_, gpointer user_data _U_)
{
    gtk_widget_set_state(eb, GTK_STATE_NORMAL);

    return FALSE;
}


static gboolean
ip_label_press_cb(GtkWidget *widget, GdkEvent *event _U_, gpointer data)
{
  GtkWidget *ip_lb            = (GtkWidget *)g_object_get_data(G_OBJECT(widget), CAPTURE_IF_IP_ADDR_LABEL);
  GSList    *addr_list        = (GSList *)data;
  GSList    *curr_addr, *start_addr;
  if_addr_t *addr;
  guint      selected_ip_addr = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(ip_lb), CAPTURE_IF_SELECTED_IP_ADDR));

  /* Select next IP address */
  start_addr = g_slist_nth(addr_list, selected_ip_addr);
  for (;;) {
    selected_ip_addr++;
    if (g_slist_length(addr_list) <= selected_ip_addr) {
      /* Wrap around */
      selected_ip_addr = 0;
    }
    curr_addr = g_slist_nth(addr_list, selected_ip_addr);
    if (curr_addr == start_addr) {
      /* We wrapped all the way around */
      break;
    }

    addr = (if_addr_t *)curr_addr->data;
    switch (addr->ifat_type) {

    case IF_AT_IPv4:
    case IF_AT_IPv6:
      goto found;

    default:
      /* In case we add non-IP addresses */
      break;
    }
  }

found:
  set_ip_addr_label(addr_list, ip_lb, selected_ip_addr);

  return FALSE;
}

static void
capture_if_stop_cb(GtkWidget *w _U_, gpointer d _U_)
{
  guint         ifs;
  if_dlg_data_t data;

  for (ifs = 0; ifs < if_array->len; ifs++) {
    data = g_array_index(if_array, if_dlg_data_t, ifs);
    if (data.hidden) {
      continue;
    }
    gtk_widget_set_sensitive(data.choose_bt, TRUE);
    if_array = g_array_remove_index(if_array, ifs);
    g_array_insert_val(if_array, ifs, data);
  }
  capture_stop_cb(NULL, NULL);
}

static void
make_if_array(void)
{
  if_dlg_data_t data;
  guint         i;

  if_array = g_array_new(FALSE, FALSE, sizeof(if_dlg_data_t));

  for (i = 0; i < global_capture_opts.all_ifaces->len; i++) {
     data.device_lb  = NULL;
     data.descr_lb   = NULL;
     data.ip_lb      = NULL;
     data.curr_lb    = NULL;
     data.last_lb    = NULL;
     data.choose_bt  = NULL;
#ifdef _WIN32
     data.details_bt = NULL;
#endif
     data.hidden     = FALSE;
     g_array_append_val(if_array, data);
  }
}

/*
 * If this is Windows, is WinPcap loaded?  If not, pop up a dialog noting
 * that fact and return FALSE, as we can't capture traffic.
 *
 * Otherwise (not Windows or WinPcap loaded), are there any interfaces?
 * If not, pop up a dialog noting that fact and return FALSE, as there
 * are no interfaces on which to capture traffic.
 *
 * Otherwise, return TRUE, as we can capture.
 */
static gboolean
can_capture(void)
{
#ifdef _WIN32
  /* Is WPcap loaded? */
  if (!has_wpcap) {
    char *detailed_err;

    detailed_err = cant_load_winpcap_err("Wireshark");
    simple_error_message_box("%s", detailed_err);
    g_free(detailed_err);
    return FALSE;
  }
#endif

  if (global_capture_opts.all_ifaces->len == 0) {
    simple_error_message_box("There are no interfaces on which a capture can be done.");
    return FALSE;
  }

  return TRUE;
}

static void
capture_if_refresh_if_list(void)
{
  GtkWidget      *if_vb, *if_grid, *icon, *if_lb, *eb;
  GtkRequisition  requisition;
  int             row = 0, height = 0;
  guint           ifs;
  interface_t     device;
  const gchar    *addr_str;
  gchar          *user_descr;
  if_dlg_data_t   data;

  if (!can_capture()) {
    /* No interfaces or, on Windows, no WinPcap; we've already popped
       up a message, so just get rid of the interface dialog. */
    destroy_if_window();
    return;
  }

  if (cap_if_sw) {
    /* First, get rid of the old interface list, and stop updating
       the statistics on it. */
    gtk_container_remove(GTK_CONTAINER(cap_if_top_vb), cap_if_sw);
    capture_stat_stop(sc);
    g_source_remove(timer_id);
  }

  /* Now construct the new interface list, and start updating the
     statistics on it. */

  make_if_array();

  cap_if_sw = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cap_if_sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(cap_if_top_vb), cap_if_sw, TRUE, TRUE, 0);

  if_vb = gtk_alignment_new(0.0F, 0.0F, 1.0F, 0.0F);
  gtk_container_set_border_width(GTK_CONTAINER(if_vb), 5);
#if ! GTK_CHECK_VERSION(3,8,0)
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(cap_if_sw), if_vb);
#else
  gtk_container_add(GTK_CONTAINER(cap_if_sw), if_vb);
#endif

  if_grid = ws_gtk_grid_new();
  ws_gtk_grid_set_row_spacing(GTK_GRID(if_grid), 3);
  ws_gtk_grid_set_column_spacing(GTK_GRID(if_grid), 3);
  gtk_container_add(GTK_CONTAINER(if_vb), if_grid);

  /* This is the icon column, used to display which kind of interface we have */
  if_lb = gtk_label_new("");
  ws_gtk_grid_attach_extended(GTK_GRID(if_grid), if_lb, 0, row, 1, 1,
      (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);

  if_lb = gtk_label_new("Device");
  ws_gtk_grid_attach_defaults(GTK_GRID(if_grid), if_lb, 1, row, 3, 1);

  if_lb = gtk_label_new("Description");
  ws_gtk_grid_attach_defaults(GTK_GRID(if_grid), if_lb, 4, row, 1, 1);

  if_lb = gtk_label_new(" IP ");
  ws_gtk_grid_attach_extended(GTK_GRID(if_grid), if_lb, 5, row, 1, 1,
      (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);

  if_lb = gtk_label_new("Packets");
  ws_gtk_grid_attach_extended(GTK_GRID(if_grid), if_lb, 6, row, 1, 1,
      (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);

  if_lb = gtk_label_new(" Packets/s ");
  ws_gtk_grid_attach_extended(GTK_GRID(if_grid), if_lb, 7, row, 1, 1,
      (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);
  row++;

  height += 30;
  /* Start gathering statistics (using dumpcap) */
  sc = capture_stat_start(&global_capture_opts);

  /* List the interfaces */
  for (ifs = 0; ifs < global_capture_opts.all_ifaces->len; ifs++) {
    device = g_array_index(global_capture_opts.all_ifaces, interface_t, ifs);
    data = g_array_index(if_array, if_dlg_data_t, ifs);
    /* Continue if capture device is hidden */
    if (device.hidden) {
      data.hidden = TRUE;
      if_array = g_array_remove_index(if_array, ifs);
      g_array_insert_val(if_array, ifs, data);
      continue;
    }
    data.choose_bt = gtk_check_button_new();
    ws_gtk_grid_attach_extended(GTK_GRID(if_grid), data.choose_bt, 0, row, 1, 1,
      (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);
    if (gbl_capture_in_progress) {
      gtk_widget_set_sensitive(data.choose_bt, FALSE);
    } else {
      gtk_widget_set_sensitive(data.choose_bt, TRUE);
    }
    gtk_toggle_button_set_active((GtkToggleButton *)data.choose_bt, device.selected);
    g_signal_connect(data.choose_bt, "toggled", G_CALLBACK(store_selected), device.name);
    /* Kind of adaptor (icon) */
    icon = capture_get_if_icon(&(device));
    ws_gtk_grid_attach_extended(GTK_GRID(if_grid), icon, 1, row, 1, 1,
      (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);

     /* device name */
#ifdef _WIN32
  /*
   * On Windows, device names are generally not meaningful - NT 5
   * uses long blobs with GUIDs in them, for example - so instead we use
   * the display name which should match that shown in ncpa.cpl
   */
    data.device_lb = gtk_label_new(device.display_name);
#else
    data.device_lb = gtk_label_new(device.name);
#endif
    gtk_misc_set_alignment(GTK_MISC(data.device_lb), 0.0f, 0.5f);
    if (strlen(gtk_label_get_text(GTK_LABEL(data.device_lb))) > SOFT_LABEL_LEN) {
        gtk_label_set_ellipsize(GTK_LABEL(data.device_lb), PANGO_ELLIPSIZE_MIDDLE);
        gtk_label_set_width_chars(GTK_LABEL(data.device_lb), SOFT_LABEL_LEN);
    }
    ws_gtk_grid_attach_defaults(GTK_GRID(if_grid), data.device_lb, 2, row, 2, 1);

    /* description */
    user_descr = capture_dev_user_descr_find(device.name);
    if (user_descr) {
      data.descr_lb = gtk_label_new(user_descr);
      g_free (user_descr);
    } else {
      if (device.if_info.vendor_description)
        data.descr_lb = gtk_label_new(device.if_info.vendor_description);
      else
        data.descr_lb = gtk_label_new("");
    }
    gtk_misc_set_alignment(GTK_MISC(data.descr_lb), 0.0f, 0.5f);
    if (strlen(gtk_label_get_text(GTK_LABEL(data.descr_lb))) > SOFT_LABEL_LEN) {
        gtk_label_set_ellipsize(GTK_LABEL(data.descr_lb), PANGO_ELLIPSIZE_MIDDLE);
        gtk_label_set_width_chars(GTK_LABEL(data.descr_lb), SOFT_LABEL_LEN);
    }
    ws_gtk_grid_attach_defaults(GTK_GRID(if_grid), data.descr_lb, 4, row, 1, 1);

    /* IP address */
    /* Only one IP address will be shown, start with the first */
    data.ip_lb = gtk_label_new("");
    addr_str = set_ip_addr_label (device.if_info.addrs, data.ip_lb, 0);
    if (addr_str) {
      gtk_widget_set_sensitive(data.ip_lb, TRUE);
    } else {
      gtk_widget_set_sensitive(data.ip_lb, FALSE);
    }
    eb = gtk_event_box_new ();
    gtk_container_add(GTK_CONTAINER(eb), data.ip_lb);
    ws_gtk_grid_attach_extended(GTK_GRID(if_grid), eb, 5, row, 1, 1,
      (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);
    if (get_ip_addr_count(device.if_info.addrs) > 1) {
      /* More than one IP address, make it possible to toggle */
      g_object_set_data(G_OBJECT(eb), CAPTURE_IF_IP_ADDR_LABEL, data.ip_lb);
      gtk_widget_set_tooltip_text(eb, "Click to see additional IP addresses");
      g_signal_connect(eb, "enter-notify-event", G_CALLBACK(ip_label_enter_cb), NULL);
      g_signal_connect(eb, "leave-notify-event", G_CALLBACK(ip_label_leave_cb), NULL);
      g_signal_connect(eb, "button-press-event", G_CALLBACK(ip_label_press_cb), device.if_info.addrs);
    }

    /* packets */
    data.curr_lb = gtk_label_new("-");
    ws_gtk_grid_attach_extended(GTK_GRID(if_grid), data.curr_lb, 6, row, 1, 1,
      (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);

    /* packets/s */
    data.last_lb = gtk_label_new("-");
    ws_gtk_grid_attach_extended(GTK_GRID(if_grid), data.last_lb, 7, row, 1, 1,
      (GtkAttachOptions)0, (GtkAttachOptions)0, 0, 0);

    /* details button */
#ifdef _WIN32
    data.details_bt = ws_gtk_button_new_from_stock(WIRESHARK_STOCK_CAPTURE_DETAILS);
    gtk_widget_set_tooltip_text(data.details_bt, "Open the capture details dialog of this interface.");
    ws_gtk_grid_attach_extended(GTK_GRID(if_grid), data.details_bt, 8, row, 1, 1, 0, 0, 0, 0);
    if (capture_if_has_details(device.name)) {
      g_signal_connect(data.details_bt, "clicked", G_CALLBACK(capture_details_cb), device.name);
    } else {
      gtk_widget_set_sensitive(data.details_bt, FALSE);
    }
#endif
    if_array = g_array_remove_index(if_array, ifs);
    g_array_insert_val(if_array, ifs, data);

    row++;
    if (row <= 20) {
      /* Add up to 20 rows of interfaces, otherwise the window may become too high */
#ifdef _WIN32
      gtk_widget_get_preferred_size(GTK_WIDGET(data.details_bt), &requisition, NULL);
#else
      gtk_widget_get_preferred_size(GTK_WIDGET(data.choose_bt), &requisition, NULL);
#endif
      height += requisition.height;
    }
  }

  gtk_widget_get_preferred_size(GTK_WIDGET(close_bt), &requisition, NULL);
  /* height + static offset + grid spacing */
  height += requisition.height + 40 + (row * 3);

  gtk_window_set_default_size(GTK_WINDOW(cap_if_w), -1, height);

  gtk_widget_show_all(cap_if_w);

  /* update the interface list every 1000ms */
  timer_id = g_timeout_add(1000, update_all, sc);
}

/* start getting capture stats from all interfaces */
void
capture_if_cb(GtkWidget *w _U_, gpointer d _U_)
{
  GtkWidget *bbox,
            *help_bt;
#ifdef HAVE_AIRPCAP
  GtkWidget *decryption_cb;
#endif

  if (cap_if_w != NULL) {
    /* There's already a "Capture Interfaces" dialog box; reactivate it. */
    reactivate_window(cap_if_w);
    return;
  }

  if (!can_capture()) {
    /* No interfaces or, on Windows, no WinPcap; just give up. */
    return;
  }

#ifdef HAVE_AIRPCAP
  /* LOAD AIRPCAP INTERFACES */

  /* If no airpcap interface is present, gray everything */
  if (airpcap_if_active == NULL) {
    if (g_airpcap_if_list == NULL) {
      /*No airpcap device found */
      airpcap_enable_toolbar_widgets(wireless_tb,FALSE);
    } else {
      /* default adapter is not airpcap... or is airpcap but is not found*/
      if (airpcap_if_active){
        /* XXX can never happen? */
        airpcap_set_toolbar_stop_capture(airpcap_if_active);
      }
      airpcap_enable_toolbar_widgets(wireless_tb,FALSE);
    }
  }
  if (airpcap_if_active){
    decryption_cb = (GtkWidget *)g_object_get_data(G_OBJECT(wireless_tb),AIRPCAP_TOOLBAR_DECRYPTION_KEY);
    /* XXXX update_decryption_mode_list() trigers a rescan, should only be done if the mode is changed */
    update_decryption_mode_list(decryption_cb);
    airpcap_set_toolbar_start_capture(airpcap_if_active);
  }
#endif

  cap_if_w = dlg_window_new("Wireshark: Capture Interfaces");  /* transient_for top_level  */
  /* XXX: Note that on Windows the following is effectively a no-op; See dlg_window_new(). */
  /*      This means, on Windows, if the top_level Wireshark window is destroyed when      */
  /*      the capture_interfaces dialog window is open, that the dumpcap process will not  */
  /*      be stopped since the destroy callback for the capture_interfaces dialog window   */
  /*      will not be called.                                                              */
  /*      ToDo: Fix this !                                                                 */
  gtk_window_set_destroy_with_parent (GTK_WINDOW(cap_if_w), TRUE);

  cap_if_top_vb = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, DLG_UNRELATED_SPACING, FALSE);
  gtk_container_add(GTK_CONTAINER(cap_if_w), cap_if_top_vb);
  gtk_container_set_border_width(GTK_CONTAINER(cap_if_top_vb), DLG_OUTER_MARGIN);

  /* Button row: close, help, stop, start, and options button */
  bbox = dlg_button_row_new(GTK_STOCK_HELP, WIRESHARK_STOCK_CAPTURE_START, WIRESHARK_STOCK_CAPTURE_OPTIONS, WIRESHARK_STOCK_CAPTURE_STOP, GTK_STOCK_CLOSE, NULL);

  gtk_box_pack_end(GTK_BOX(cap_if_top_vb), bbox, FALSE, FALSE, 0);
  help_bt = (GtkWidget *)g_object_get_data(G_OBJECT(bbox), GTK_STOCK_HELP);
  g_signal_connect(help_bt, "clicked", G_CALLBACK(topic_cb), (gpointer)(HELP_CAPTURE_INTERFACES_DIALOG));

  stop_bt = (GtkWidget *)g_object_get_data(G_OBJECT(bbox), WIRESHARK_STOCK_CAPTURE_STOP);
  g_signal_connect(stop_bt, "clicked", G_CALLBACK(capture_if_stop_cb), NULL);
  close_bt = (GtkWidget *)g_object_get_data(G_OBJECT(bbox), GTK_STOCK_CLOSE);
  window_set_cancel_button(cap_if_w, close_bt, window_cancel_button_cb);
  gtk_widget_set_tooltip_text(close_bt, "Close this window.");
  options_bt = (GtkWidget *)g_object_get_data(G_OBJECT(bbox), WIRESHARK_STOCK_CAPTURE_OPTIONS);
  g_signal_connect(options_bt, "clicked", G_CALLBACK(capture_prepare_cb), NULL);
  capture_bt = (GtkWidget *)g_object_get_data(G_OBJECT(bbox), WIRESHARK_STOCK_CAPTURE_START);
  g_signal_connect(capture_bt, "clicked", G_CALLBACK(capture_do_cb), NULL);

  gtk_widget_grab_default(close_bt);

  g_signal_connect(cap_if_w, "delete_event", G_CALLBACK(window_delete_event_cb), NULL);
  g_signal_connect(cap_if_w, "destroy", G_CALLBACK(capture_if_destroy_cb), sc);

  capture_if_refresh_if_list();

  window_present(cap_if_w);

  set_capture_if_dialog_for_capture_in_progress(gbl_capture_in_progress);
}

gboolean interfaces_dialog_window_present(void)
{
  return (cap_if_w?TRUE:FALSE);
}

void refresh_if_window(void)
{
  if (cap_if_w) {
    capture_if_refresh_if_list();
  }
}

void select_all_interfaces(gboolean enable _U_)
{
  guint       ifs;
  interface_t device;

  for (ifs = 0; ifs < global_capture_opts.all_ifaces->len; ifs++) {
    device = g_array_index(global_capture_opts.all_ifaces, interface_t, ifs);
    update_selected_interface(device.if_info.name);
  }
}

void destroy_if_window(void)
{
  if (cap_if_w) {
    window_destroy(cap_if_w);
  }
}
#else /* HAVE_LIBPCAP */

void
set_capture_if_dialog_for_capture_in_progress(gboolean capture_in_progress _U_)
{
}

#endif /* HAVE_LIBPCAP */

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
