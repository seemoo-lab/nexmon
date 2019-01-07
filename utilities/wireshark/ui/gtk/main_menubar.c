/* main_menubar.c
 * Menu routines
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

#include <gtk/gtk.h>

#include <stdlib.h>
#include <string.h>

#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/prefs-int.h>
#include <epan/dissector_filters.h>
#include <epan/epan_dissect.h>
#include <epan/column.h>
#include <epan/stats_tree_priv.h>
#include <epan/plugin_if.h>

#include "globals.h"
#include <epan/color_filters.h>

#include "ui/commandline.h"
#include "ui/main_statusbar.h"
#include "ui/preference_utils.h"
#include "ui/recent.h"
#include "ui/recent_utils.h"
#include "ui/simple_dialog.h"
#include "ui/software_update.h"
#include <wsutil/utf8_entities.h>

#include "ui/gtk/gui_stat_menu.h"
#include "ui/gtk/about_dlg.h"
#include "ui/gtk/capture_dlg.h"
#include "ui/gtk/capture_if_dlg.h"
#include "ui/gtk/color_dlg.h"
#include "ui/gtk/export_object_dlg.h"
#include "ui/gtk/filter_dlg.h"
#include "ui/gtk/profile_dlg.h"
#include "ui/gtk/dlg_utils.h"
#include "ui/gtk/capture_file_dlg.h"
#include "ui/gtk/fileset_dlg.h"
#include "ui/gtk/file_import_dlg.h"
#include "ui/gtk/find_dlg.h"
#include "ui/gtk/goto_dlg.h"
#include "ui/gtk/summary_dlg.h"
#include "ui/gtk/prefs_dlg.h"
#include "ui/gtk/packet_win.h"
#include "ui/gtk/follow_stream.h"
#include "ui/gtk/decode_as_dlg.h"
#include "ui/gtk/help_dlg.h"
#include "ui/gtk/supported_protos_dlg.h"
#include "ui/gtk/proto_dlg.h"
#include "ui/gtk/proto_hier_stats_dlg.h"
#include "ui/gtk/keys.h"
#include "ui/gtk/stock_icons.h"
#include "ui/gtk/gtkglobals.h"
#include "ui/gtk/packet_panes.h"
#include "ui/gtk/conversations_table.h"
#include "ui/gtk/hostlist_table.h"
#include "ui/gtk/packet_history.h"
#include "ui/gtk/sctp_stat_gtk.h"
#include "ui/gtk/firewall_dlg.h"
#include "ui/gtk/macros_dlg.h"
#include "ui/gtk/export_sslkeys.h"
#include "ui/gtk/gui_stat_menu.h"
#include "ui/gtk/main.h"
#include "ui/gtk/menus.h"
#include "ui/gtk/main_menubar_private.h"
#include "ui/gtk/main_toolbar.h"
#include "ui/gtk/main_welcome.h"
#include "ui/gtk/uat_gui.h"
#include "ui/gtk/gui_utils.h"
#include "ui/gtk/manual_addr_resolv.h"
#include "ui/gtk/dissector_tables_dlg.h"
#include "ui/gtk/expert_comp_dlg.h"
#include "ui/gtk/time_shift_dlg.h"
#include "ui/gtk/edit_packet_comment_dlg.h"
#include "ui/gtk/addr_resolution_dlg.h"
#include "ui/gtk/export_pdu_dlg.h"
#include "ui/gtk/conversation_hastables_dlg.h"
#include "ui/gtk/webbrowser.h"

#include "ui/gtk/packet_list.h"
#include "ui/gtk/lbm_stream_dlg.h"
#include "ui/gtk/lbm_uimflow_dlg.h"

#ifdef HAVE_LIBPCAP
#include "capture_opts.h"
#include "ui/capture_globals.h"
#endif
#ifdef HAVE_IGE_MAC_INTEGRATION
#include <ige-mac-menu.h>
#endif

#ifdef HAVE_GTKOSXAPPLICATION
#include <gtkmacintegration/gtkosxapplication.h>
#endif

static int initialize = TRUE;
GtkActionGroup    *main_menu_bar_action_group;
static GtkUIManager *ui_manager_main_menubar = NULL;
static GtkUIManager *ui_manager_packet_list_heading = NULL;
static GtkUIManager *ui_manager_packet_list_menu = NULL;
static GtkUIManager *ui_manager_tree_view_menu = NULL;
static GtkUIManager *ui_manager_bytes_menu = NULL;
static GtkUIManager *ui_manager_statusbar_profiles_menu = NULL;
static GSList *popup_menu_list = NULL;

static GtkAccelGroup *grp;

static GList *merge_menu_items_list = NULL;
static GList *build_menubar_items_callback_list = NULL;

GtkWidget *popup_menu_object;

static void menu_open_recent_file_cmd_cb(GtkAction *action, gpointer data _U_ );
static void add_recent_items (guint merge_id, GtkUIManager *ui_manager);
static void add_tap_plugins (guint merge_id, GtkUIManager *ui_manager);

static void menus_init(void);
static void merge_menu_items(GList *node);
static void ws_menubar_external_menus(void);
static void ws_menubar_build_external_menus(void);
static void set_menu_sensitivity (GtkUIManager *ui_manager, const gchar *, gint);
static void menu_name_resolution_update_cb(GtkAction *action, gpointer data);
static void name_resolution_cb(GtkWidget *w, gpointer d, gboolean* res_flag);
static void colorize_cb(GtkWidget *w, gpointer d);
static void rebuild_protocol_prefs_menu (module_t *prefs_module_p, gboolean preferences,
        GtkUIManager *ui_menu, const char *path);

static void plugin_if_menubar_preference(gconstpointer user_data);


/*  As a general GUI guideline, we try to follow the Gnome Human Interface Guidelines, which can be found at:
    http://developer.gnome.org/projects/gup/hig/1.0/index.html

Please note: there are some differences between the Gnome HIG menu suggestions and our implementation:

File/Open Recent:   the Gnome HIG suggests putting the list of recently used files as elements into the File menuitem.
                    As this is ok for only a few items, this will become unhandy for 10 or even more list entries.
                    For this reason, we use a submenu for this.

File/Close:         the Gnome HIG suggests putting this item just above the Quit item.
                    This results in unintuitive behaviour as both Close and Quit items are very near together.
                    By putting the Close item near the open item(s), it better suggests that it will close the
                    currently opened/captured file only.
*/

static void
new_window_cb(GtkWidget *widget)
{
    new_packet_window(widget, FALSE, FALSE);
}

static void
new_window_cb_ref(GtkWidget *widget)
{
    new_packet_window(widget, TRUE, FALSE);
}

#ifdef WANT_PACKET_EDITOR
static void
edit_window_cb(GtkWidget *widget _U_)
{
    new_packet_window(widget, FALSE, TRUE);
}
#endif

static void
colorize_conversation_cb(conversation_filter_t* color_filter, int action_num)
{
    gchar *filter = NULL;
    packet_info *pi = &cfile.edt->pi;
    gchar *err_msg = NULL;

    if (action_num == 255) {
        if (!color_filters_reset_tmp(&err_msg)) {
            simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "%s", err_msg);
            g_free(err_msg);
        }
        packet_list_colorize_packets();
    } else if (cfile.current_frame) {
        if (color_filter == NULL) {
            /* colorize_conversation_cb was called from the window-menu
                * or through an accelerator key. Try to build a conversation
                * filter in the order TCP, UDP, IP, Ethernet and apply the
                * coloring */
            color_filter = find_conversation_filter("tcp");
            if ((color_filter != NULL) && (color_filter->is_filter_valid(pi)))
                filter = color_filter->build_filter_string(pi);
            if (filter == NULL) {
                color_filter = find_conversation_filter("udp");
                if ((color_filter != NULL) && (color_filter->is_filter_valid(pi)))
                    filter = color_filter->build_filter_string(pi);
            }
            if (filter == NULL) {
                color_filter = find_conversation_filter("ip");
                if ((color_filter != NULL) && (color_filter->is_filter_valid(pi)))
                    filter = color_filter->build_filter_string(pi);
            }
            if (filter == NULL) {
                color_filter = find_conversation_filter("ipv6");
                if ((color_filter != NULL) && (color_filter->is_filter_valid(pi)))
                    filter = color_filter->build_filter_string(pi);
            }
            if (filter == NULL) {
                color_filter = find_conversation_filter("eth");
                if ((color_filter != NULL) && (color_filter->is_filter_valid(pi)))
                    filter = color_filter->build_filter_string(pi);
            }
            if( filter == NULL ) {
                simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "Unable to build conversation filter.");
                return;
            }
        } else {
            filter = color_filter->build_filter_string(pi);
        }

        if (action_num == 0) {
            /* Open the "new coloring filter" dialog with the filter */
            color_display_with_filter(filter);
        } else {
            /* Set one of the temporary coloring filters */
            if (!color_filters_set_tmp(action_num, filter, FALSE, &err_msg)) {
                simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "%s", err_msg);
                g_free(err_msg);
            }
            packet_list_colorize_packets();
        }

        g_free(filter);
    }
}

static void
goto_conversation_frame(gboolean dir)
{
    gchar     *filter       = NULL;
    dfilter_t *dfcode       = NULL;
    gboolean   found_packet = FALSE;
    packet_info *pi = &cfile.edt->pi;
    conversation_filter_t* conv_filter;

    /* Try to build a conversation
     * filter in the order TCP, UDP, IP, Ethernet and apply the
     * coloring */
    conv_filter = find_conversation_filter("tcp");
    if ((conv_filter != NULL) && (conv_filter->is_filter_valid(pi)))
        filter = conv_filter->build_filter_string(pi);
    conv_filter = find_conversation_filter("udp");
    if ((conv_filter != NULL) && (conv_filter->is_filter_valid(pi)))
        filter = conv_filter->build_filter_string(pi);
    conv_filter = find_conversation_filter("ip");
    if ((conv_filter != NULL) && (conv_filter->is_filter_valid(pi)))
        filter = conv_filter->build_filter_string(pi);
    conv_filter = find_conversation_filter("ipv6");
    if ((conv_filter != NULL) && (conv_filter->is_filter_valid(pi)))
        filter = conv_filter->build_filter_string(pi);

    if( filter == NULL ) {
        statusbar_push_temporary_msg("Unable to build conversation filter.");
        g_free(filter);
        return;
    }

    if (!dfilter_compile(filter, &dfcode, NULL)) {
        /* The attempt failed; report an error. */
        statusbar_push_temporary_msg("Error compiling filter for this conversation.");
        g_free(filter);
        return;
    }

    found_packet = cf_find_packet_dfilter(&cfile, dfcode, dir?SD_BACKWARD:SD_FORWARD);

    if (!found_packet) {
        /* We didn't find a packet */
        statusbar_push_temporary_msg("No previous/next packet in conversation.");
    }

    dfilter_free(dfcode);
    g_free(filter);
}

static void
goto_next_frame_conversation_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    goto_conversation_frame(FALSE);
}

static void
goto_previous_frame_conversation_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    goto_conversation_frame(TRUE);
}



static void
copy_description_cb(GtkAction *action _U_, gpointer user_data)
{
    copy_selected_plist_cb( NULL /* widget _U_ */ , user_data, COPY_SELECTED_DESCRIPTION);
}

static void
copy_fieldname_cb(GtkAction *action _U_, gpointer user_data)
{
    copy_selected_plist_cb( NULL /* widget _U_ */ , user_data, COPY_SELECTED_FIELDNAME);
}

static void
copy_value_cb(GtkAction *action _U_, gpointer user_data)
{
    copy_selected_plist_cb( NULL /* widget _U_ */ , user_data, COPY_SELECTED_VALUE);
}

static void
copy_as_filter_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    /* match_selected_ptree_cb needs the popup_menu_object to get the right object E_DFILTER_TE_KEY */
    match_selected_ptree_cb( popup_menu_object, (MATCH_SELECTED_E)(MATCH_SELECTED_REPLACE|MATCH_SELECTED_COPY_ONLY));
}

static void
set_reftime_cb(GtkAction *action _U_, gpointer user_data)
{
    reftime_frame_cb( NULL /* widget _U_ */ , user_data, REFTIME_TOGGLE);
}

static void
find_next_ref_time_cb(GtkAction *action _U_, gpointer user_data)
{
    reftime_frame_cb( NULL /* widget _U_ */ , user_data, REFTIME_FIND_NEXT);
}

static void
find_previous_ref_time_cb(GtkAction *action _U_, gpointer user_data)
{
    reftime_frame_cb( NULL /* widget _U_ */ , user_data, REFTIME_FIND_PREV);
}

static void
menus_prefs_cb(GtkAction *action _U_, gpointer user_data)
{
    prefs_page_cb( NULL /* widget _U_ */ , user_data, PREFS_PAGE_USER_INTERFACE);
}

static void
main_toolbar_show_hide_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/MainToolbar");

    recent.main_toolbar_show = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    main_widgets_show_or_hide();

}

static void
filter_toolbar_show_hide_cb(GtkAction * action _U_, gpointer user_data _U_)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/FilterToolbar");

    recent.filter_toolbar_show = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    main_widgets_show_or_hide();
}

static void
wireless_toolbar_show_hide_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/WirelessToolbar");

    if(widget) {
        recent.wireless_toolbar_show = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    } else {
        recent.wireless_toolbar_show = FALSE;
    }
    main_widgets_show_or_hide();
}

static void
status_bar_show_hide_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/StatusBar");

    recent.statusbar_show = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    main_widgets_show_or_hide();
}
static void
packet_list_show_hide_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/PacketList");

    recent.packet_list_show = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    main_widgets_show_or_hide();
}
static void
packet_details_show_hide_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/PacketDetails");

    recent.tree_view_show = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    main_widgets_show_or_hide();
}
static void
packet_bytes_show_hide_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/PacketBytes");

    recent.byte_view_show = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
    main_widgets_show_or_hide();
}

static void
timestamp_seconds_time_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/TimeDisplayFormat/DisplaySecondsWithHoursAndMinutes");

    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))) {
        recent.gui_seconds_format = TS_SECONDS_HOUR_MIN_SEC;
    } else {
        recent.gui_seconds_format = TS_SECONDS_DEFAULT;
    }
    timestamp_set_seconds_type (recent.gui_seconds_format);

    /* This call adjusts column width */
    cf_timestamp_auto_precision(&cfile);
    packet_list_queue_draw();
}

static void
timestamp_format_new_cb (GtkRadioAction *action, GtkRadioAction *current _U_, gpointer user_data  _U_)
{
    ts_type value;

    value = (ts_type) gtk_radio_action_get_current_value (action);
    if (recent.gui_time_format != value) {
        timestamp_set_type(value);
        recent.gui_time_format = value;
        /* This call adjusts column width */
        cf_timestamp_auto_precision(&cfile);
        packet_list_queue_draw();
    }

}

static void
timestamp_precision_new_cb (GtkRadioAction *action, GtkRadioAction *current _U_, gpointer user_data _U_)
{
    gint value;

    value = gtk_radio_action_get_current_value (action);
    if (recent.gui_time_precision != value) {
        /* the actual precision will be set in packet_list_queue_draw() below */
        timestamp_set_precision(value);
        recent.gui_time_precision  = value;
        /* This call adjusts column width */
        cf_timestamp_auto_precision(&cfile);
        packet_list_queue_draw();
    }
}

static void
view_menu_en_for_MAC_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/NameResolution/EnableforMACLayer");

    if (!widget){
        g_warning("view_menu_en_for_MAC_cb: No widget found");
    }else{
        name_resolution_cb( widget , user_data, &gbl_resolv_flags.mac_name);
    }
}

static void
view_menu_en_for_network_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/NameResolution/EnableforNetworkLayer");

    if (!widget){
        g_warning("view_menu_en_for_network_cb: No widget found");
    }else{
        name_resolution_cb( widget , user_data, &gbl_resolv_flags.network_name);
    }
}

static void
view_menu_en_for_transport_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/NameResolution/EnableforTransportLayer");

    if (!widget){
        g_warning("view_menu_en_for_transport_cb: No widget found");
    }else{
        name_resolution_cb( widget , user_data, &gbl_resolv_flags.transport_name);
    }
}

static void
view_menu_en_use_external_resolver_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/NameResolution/UseExternalNetworkNameResolver");

    if (!widget){
        g_warning("view_menu_en_use_external_resolver_cb: No widget found");
    }else{
        name_resolution_cb( widget , user_data, &gbl_resolv_flags.use_external_net_name_resolver);
    }
}

static void
view_menu_colorize_pkt_lst_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/ColorizePacketList");

    if (!widget){
        g_warning("view_menu_colorize_pkt_lst_cb: No widget found");
    }else{
        colorize_cb( widget , user_data);
    }

}

#ifdef HAVE_LIBPCAP
static void
view_menu_auto_scroll_live_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/AutoScrollinLiveCapture");

    if (!widget){
        g_warning("view_menu_auto_scroll_live_cb: No widget found");
    }else{
        main_auto_scroll_live_changed(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)));
    }
}
#endif

static void
view_menu_color_conv_color1_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    colorize_conversation_cb(NULL, 1);
}

static void
view_menu_color_conv_color2_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    colorize_conversation_cb(NULL, 2);
}

static void
view_menu_color_conv_color3_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    colorize_conversation_cb(NULL, 3);
}

static void
view_menu_color_conv_color4_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    colorize_conversation_cb(NULL, 4);
}

static void
view_menu_color_conv_color5_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    colorize_conversation_cb(NULL, 5);
}

static void
view_menu_color_conv_color6_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    colorize_conversation_cb(NULL, 6);
}

static void
view_menu_color_conv_color7_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    colorize_conversation_cb(NULL, 7);
}

static void
view_menu_color_conv_color8_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    colorize_conversation_cb(NULL, 8);
}

static void
view_menu_color_conv_color9_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    colorize_conversation_cb(NULL, 9);
}

static void
view_menu_color_conv_color10_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    colorize_conversation_cb(NULL, 10);
}

static void
view_menu_color_conv_new_rule_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    colorize_conversation_cb(NULL, 0);
}

static void
view_menu_reset_coloring_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    colorize_conversation_cb(NULL, 255);
}

/*
 * TODO Move this menu to capture_if_dlg.c ?
 */
#ifdef HAVE_LIBPCAP
static void
capture_if_action_cb(GtkAction *action _U_, gpointer user_data)
{
    capture_if_cb(NULL /* GtkWidget *w _U_ */, user_data);
}

static void
capture_prep_action_cb(GtkAction *action _U_, gpointer user_data)
{
    capture_prep_cb(NULL /* GtkWidget *w _U_ */, user_data);
}

static void
capture_start_action_cb(GtkAction *action _U_, gpointer user_data)
{
    capture_start_cb(NULL /* GtkWidget *w _U_ */, user_data);
}

static void
capture_stop_action_cb(GtkAction *action _U_, gpointer user_data)
{
    capture_stop_cb(NULL /* GtkWidget *w _U_ */, user_data);
}

static void
capture_restart_action_cb(GtkAction *action _U_, gpointer user_data)
{
    capture_restart_cb(NULL /* GtkWidget *w _U_ */, user_data);
}

static void
capture_filters_action_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    cfilter_dialog_cb(NULL /* GtkWidget *w _U_ */);
}

/*
 * We've been asked to rescan the system looking for interfaces.
 */
static void
refresh_interfaces_action_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    refresh_local_interface_lists();
}
#endif /* HAVE_LIBPCAP */

static void
help_menu_cont_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/*widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(HELP_CONTENT));
}

static void
help_menu_faq_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/*widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(ONLINEPAGE_FAQ));
}

static void
help_menu_ask_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/*widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(ONLINEPAGE_ASK));
}

static void
help_menu_wireshark_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/*widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(LOCALPAGE_MAN_WIRESHARK));
}

static void
help_menu_wireshark_flt_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/*widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(LOCALPAGE_MAN_WIRESHARK_FILTER));
}

static void
help_menu_Capinfos_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/*widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(LOCALPAGE_MAN_CAPINFOS));
}

static void
help_menu_Dumpcap_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/*widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(LOCALPAGE_MAN_DUMPCAP));
}

static void
help_menu_Editcap_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/* widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(LOCALPAGE_MAN_EDITCAP));
}

static void
help_menu_Mergecap_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/*widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(LOCALPAGE_MAN_MERGECAP));
}

static void
help_menu_RawShark_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/*widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(LOCALPAGE_MAN_RAWSHARK));
}

static void
help_menu_Reorder_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/*widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(LOCALPAGE_MAN_REORDERCAP));
}

static void
help_menu_Text2pcap_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/* widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(LOCALPAGE_MAN_TEXT2PCAP));
}

static void
help_menu_TShark_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/*widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(LOCALPAGE_MAN_TSHARK));
}

static void
help_menu_Website_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/* widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(ONLINEPAGE_HOME));
}

static void
help_menu_Wiki_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/* widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(ONLINEPAGE_WIKI));
}

static void
help_menu_Downloads_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb(NULL/* widget _U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(ONLINEPAGE_DOWNLOAD));
}

static void
help_menu_SampleCaptures_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    topic_menu_cb( NULL/* widget_U_ */, NULL /*GdkEventButton *event _U_*/, GINT_TO_POINTER(ONLINEPAGE_SAMPLE_FILES));
}

#ifdef HAVE_SOFTWARE_UPDATE
static void
check_for_updates_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    software_update_check();
}
#endif /* HAVE_SOFTWARE_UPDATE */

static const char *ui_desc_menubar =
"<ui>\n"
"  <menubar name ='Menubar'>\n"
"    <menu name= 'FileMenu' action='/File'>\n"
"      <menuitem name='Open' action='/File/Open'/>\n"
"      <menu name='OpenRecent' action='/File/OpenRecent'>\n"
"         <placeholder name='RecentFiles'/>\n"
"      </menu>\n"
"      <menuitem name='Merge' action='/File/Merge'/>\n"
"      <menuitem name='ImportFromHexDump' action='/File/ImportFromHexDump'/>\n"
"      <menuitem name='Close' action='/File/Close'/>\n"
"      <separator/>\n"
"      <menuitem name='Save' action='/File/Save'/>\n"
"      <menuitem name='SaveAs' action='/File/SaveAs'/>\n"
"      <separator/>\n"
"      <menu name= 'Set' action='/File/Set'>\n"
"        <menuitem name='ListFiles' action='/File/Set/ListFiles'/>\n"
"        <menuitem name='NextFile' action='/File/Set/NextFile'/>\n"
"        <menuitem name='PreviousFile' action='/File/Set/PreviousFile'/>\n"
"      </menu>\n"
"      <separator/>\n"
"      <menuitem name='ExportSpecifiedPackets' action='/File/ExportSpecifiedPackets'/>\n"
"      <menu name= 'ExportPacketDissections' action='/File/ExportPacketDissections'>\n"
"        <menuitem name='AsTxt' action='/File/ExportPacketDissections/Text'/>\n"
"        <menuitem name='AsPostScript' action='/File/ExportPacketDissections/PostScript'/>\n"
"        <menuitem name='AsCSV' action='/File/ExportPacketDissections/CSV'/>\n"
"        <menuitem name='AsCArrays' action='/File/ExportPacketDissections/CArrays'/>\n"
"        <separator/>\n"
"        <menuitem name='AsPSML' action='/File/ExportPacketDissections/PSML'/>\n"
"        <menuitem name='AsPDML' action='/File/ExportPacketDissections/PDML'/>\n"
"        <menuitem name='AsJSON' action='/File/ExportPacketDissections/JSON'/>\n"
"        <separator/>\n"
"      </menu>\n"
"      <menuitem name='ExportSelectedPacketBytes' action='/File/ExportSelectedPacketBytes'/>\n"
"      <menuitem name='ExportPDUs' action='/File/ExportPDUs'/>\n"
"      <menuitem name='ExportSSLSessionKeys' action='/File/ExportSSLSessionKeys'/>\n"
"      <menu name= 'ExportObjects' action='/File/ExportObjects'>\n"
"        <menuitem name='HTTP' action='/File/ExportObjects/HTTP'/>\n"
"        <menuitem name='DICOM' action='/File/ExportObjects/DICOM'/>\n"
"        <menuitem name='SMB' action='/File/ExportObjects/SMB'/>\n"
"        <menuitem name='TFTP' action='/File/ExportObjects/TFTP'/>\n"
"      </menu>\n"
"      <separator/>\n"
"      <menuitem name='Print' action='/File/Print'/>\n"
"      <separator/>\n"
"        <menuitem name='Quit' action='/File/Quit'/>\n"
"    </menu>\n"
"    <menu name= 'EditMenu' action='/Edit'>\n"
"        <menu name= 'Copy' action='/Edit/Copy'>\n"
"          <menuitem name='Description' action='/Edit/Copy/Description'/>\n"
"          <menuitem name='Fieldname' action='/Edit/Copy/Fieldname'/>\n"
"          <menuitem name='Value' action='/Edit/Copy/Value'/>\n"
"          <separator/>\n"
"          <menuitem name='AsFilter' action='/Edit/Copy/AsFilter'/>\n"
"        </menu>\n"
"        <menuitem name='FindPacket' action='/Edit/FindPacket'/>\n"
"        <menuitem name='FindNext' action='/Edit/FindNext'/>\n"
"        <menuitem name='FindPrevious' action='/Edit/FindPrevious'/>\n"
"        <separator/>\n"
"        <menuitem name='MarkPacket' action='/Edit/MarkPacket'/>\n"
"        <menuitem name='MarkAllDisplayedPackets' action='/Edit/MarkAllDisplayedPackets'/>\n"
"        <menuitem name='UnmarkAllDisplayedPackets' action='/Edit/UnmarkAllDisplayedPackets'/>\n"
"        <menuitem name='FindNextMark' action='/Edit/FindNextMark'/>\n"
"        <menuitem name='FindPreviousMark' action='/Edit/FindPreviousMark'/>\n"
"        <separator/>\n"
"        <menuitem name='IgnorePacket' action='/Edit/IgnorePacket'/>\n"
"        <menuitem name='IgnoreAllDisplayedPackets' action='/Edit/IgnoreAllDisplayedPackets'/>\n"
"        <menuitem name='Un-IgnoreAllPackets' action='/Edit/Un-IgnoreAllPackets'/>\n"
"        <separator/>\n"
"        <menuitem name='SetTimeReference' action='/Edit/SetTimeReference'/>\n"
"        <menuitem name='Un-TimeReferenceAllPackets' action='/Edit/Un-TimeReferenceAllPackets'/>\n"
"        <menuitem name='FindNextTimeReference' action='/Edit/FindNextTimeReference'/>\n"
"        <menuitem name='FindPreviousTimeReference' action='/Edit/FindPreviousTimeReference'/>\n"
"        <separator/>\n"
"        <menuitem name='TimeShift' action='/Edit/TimeShift'/>\n"
"        <separator/>\n"
#ifdef WANT_PACKET_EDITOR
"        <menuitem name='EditPacket' action='/Edit/EditPacket'/>\n"
#endif
"        <menuitem name='AddEditPktComment' action='/Edit/AddEditPktComment'/>\n"
"        <menuitem name='AddEditCaptureComment' action='/Edit/AddEditCaptureComment'/>\n"
"        <separator/>\n"
"        <menuitem name='ConfigurationProfiles' action='/Edit/ConfigurationProfiles'/>\n"
"        <menuitem name='Preferences' action='/Edit/Preferences'/>\n"
"    </menu>\n"
"    <menu name= 'ViewMenu' action='/View'>\n"
"      <menuitem name='MainToolbar' action='/View/MainToolbar'/>\n"
"      <menuitem name='FilterToolbar' action='/View/FilterToolbar'/>\n"
"      <menuitem name='WirelessToolbar' action='/View/WirelessToolbar'/>\n"
"      <menuitem name='StatusBar' action='/View/StatusBar'/>\n"
"      <separator/>\n"
"      <menuitem name='PacketList' action='/View/PacketList'/>\n"
"      <menuitem name='PacketDetails' action='/View/PacketDetails'/>\n"
"      <menuitem name='PacketBytes' action='/View/PacketBytes'/>\n"
"      <separator/>\n"
"      <menu name= 'TimeDisplayFormat' action='/View/TimeDisplayFormat'>\n"
"        <menuitem name='DateYMDandTimeofDay' action='/View/TimeDisplayFormat/DateYMDandTimeofDay'/>\n"
"        <menuitem name='DateYDOYandTimeofDay' action='/View/TimeDisplayFormat/DateYDOYandTimeofDay'/>\n"
"        <menuitem name='TimeofDay' action='/View/TimeDisplayFormat/TimeofDay'/>\n"
"        <menuitem name='SecondsSinceEpoch' action='/View/TimeDisplayFormat/SecondsSinceEpoch'/>\n"
"        <menuitem name='SecondsSinceBeginningofCapture' action='/View/TimeDisplayFormat/SecondsSinceBeginningofCapture'/>\n"
"        <menuitem name='SecondsSincePreviousCapturedPacket' action='/View/TimeDisplayFormat/SecondsSincePreviousCapturedPacket'/>\n"
"        <menuitem name='SecondsSincePreviousDisplayedPacket' action='/View/TimeDisplayFormat/SecondsSincePreviousDisplayedPacket'/>\n"
"        <menuitem name='UTCDateYMDandTimeofDay' action='/View/TimeDisplayFormat/UTCDateYMDandTimeofDay'/>\n"
"        <menuitem name='UTCDateYDOYandTimeofDay' action='/View/TimeDisplayFormat/UTCDateYDOYandTimeofDay'/>\n"
"        <menuitem name='UTCTimeofDay' action='/View/TimeDisplayFormat/UTCTimeofDay'/>\n"
"        <separator/>\n"
"        <menuitem name='FileFormatPrecision-Automatic' action='/View/TimeDisplayFormat/FileFormatPrecision-Automatic'/>\n"
"        <menuitem name='FileFormatPrecision-Seconds' action='/View/TimeDisplayFormat/FileFormatPrecision-Seconds'/>\n"
"        <menuitem name='FileFormatPrecision-Deciseconds' action='/View/TimeDisplayFormat/FileFormatPrecision-Deciseconds'/>\n"
"        <menuitem name='FileFormatPrecision-Centiseconds' action='/View/TimeDisplayFormat/FileFormatPrecision-Centiseconds'/>\n"
"        <menuitem name='FileFormatPrecision-Milliseconds' action='/View/TimeDisplayFormat/FileFormatPrecision-Milliseconds'/>\n"
"        <menuitem name='FileFormatPrecision-Microseconds' action='/View/TimeDisplayFormat/FileFormatPrecision-Microseconds'/>\n"
"        <menuitem name='FileFormatPrecision-Nanoseconds' action='/View/TimeDisplayFormat/FileFormatPrecision-Nanoseconds'/>\n"
"        <separator/>\n"
"        <menuitem name='DisplaySecondsWithHoursAndMinutes' action='/View/TimeDisplayFormat/DisplaySecondsWithHoursAndMinutes'/>\n"
"      </menu>\n"
"      <menu name= 'NameResolution' action='/View/NameResolution'>\n"
"         <menuitem name='ResolveName' action='/View/NameResolution/ResolveName'/>\n"
"         <menuitem name='ManuallyResolveName' action='/View/NameResolution/ManuallyResolveName'/>\n"
"         <separator/>\n"
"         <menuitem name='EnableforMACLayer' action='/View/NameResolution/EnableforMACLayer'/>\n"
"         <menuitem name='EnableforTransportLayer' action='/View/NameResolution/EnableforTransportLayer'/>\n"
"         <menuitem name='EnableforNetworkLayer' action='/View/NameResolution/EnableforNetworkLayer'/>\n"
"         <menuitem name='UseExternalNetworkNameResolver' action='/View/NameResolution/UseExternalNetworkNameResolver'/>\n"
"      </menu>\n"
"      <menuitem name='ColorizePacketList' action='/View/ColorizePacketList'/>\n"
#ifdef HAVE_LIBPCAP
"      <menuitem name='AutoScrollinLiveCapture' action='/View/AutoScrollinLiveCapture'/>\n"
#endif
"      <separator/>\n"
"      <menuitem name='ZoomIn' action='/View/ZoomIn'/>\n"
"      <menuitem name='ZoomOut' action='/View/ZoomOut'/>\n"
"      <menuitem name='NormalSize' action='/View/NormalSize'/>\n"
"      <separator/>\n"
"      <menuitem name='ResizeAllColumns' action='/View/ResizeAllColumns'/>\n"
"      <menuitem name='DisplayedColumns' action='/View/DisplayedColumns'/>\n"
"      <separator/>\n"
"      <menuitem name='ExpandSubtrees' action='/View/ExpandSubtrees'/>\n"
"      <menuitem name='CollapseSubtrees' action='/View/CollapseSubtrees'/>\n"
"      <menuitem name='ExpandAll' action='/View/ExpandAll'/>\n"
"      <menuitem name='CollapseAll' action='/View/CollapseAll'/>\n"
"      <separator/>\n"
"      <menu name= 'ColorizeConversation' action='/View/ColorizeConversation'>\n"
"         <menuitem name='Color1' action='/View/ColorizeConversation/Color 1'/>\n"
"         <menuitem name='Color2' action='/View/ColorizeConversation/Color 2'/>\n"
"         <menuitem name='Color3' action='/View/ColorizeConversation/Color 3'/>\n"
"         <menuitem name='Color4' action='/View/ColorizeConversation/Color 4'/>\n"
"         <menuitem name='Color5' action='/View/ColorizeConversation/Color 5'/>\n"
"         <menuitem name='Color6' action='/View/ColorizeConversation/Color 6'/>\n"
"         <menuitem name='Color7' action='/View/ColorizeConversation/Color 7'/>\n"
"         <menuitem name='Color8' action='/View/ColorizeConversation/Color 8'/>\n"
"         <menuitem name='Color9' action='/View/ColorizeConversation/Color 9'/>\n"
"         <menuitem name='Color10' action='/View/ColorizeConversation/Color 10'/>\n"
"         <menuitem name='NewColoringRule' action='/View/ColorizeConversation/NewColoringRule'/>\n"
"      </menu>\n"
"      <separator/>\n"
"      <menuitem name='ResetColoring1-10' action='/View/ResetColoring1-10'/>\n"
"      <menuitem name='ColoringRules' action='/View/ColoringRules'/>\n"
"      <separator/>\n"
"      <menuitem name='ShowPacketinNewWindow' action='/View/ShowPacketinNewWindow'/>\n"
"      <menuitem name='Reload' action='/View/Reload'/>\n"
"    </menu>\n"
"    <menu name= 'GoMenu' action='/Go'>\n"
"      <menuitem name='Back' action='/Go/Back'/>\n"
"      <menuitem name='Forward' action='/Go/Forward'/>\n"
"      <menuitem name='Goto' action='/Go/Goto'/>\n"
"      <menuitem name='GotoCorrespondingPacket' action='/Go/GotoCorrespondingPacket'/>\n"
"      <separator/>\n"
"      <menuitem name='PreviousPacket' action='/Go/PreviousPacket'/>\n"
"      <menuitem name='NextPacket' action='/Go/NextPacket'/>\n"
"      <menuitem name='FirstPacket' action='/Go/FirstPacket'/>\n"
"      <menuitem name='LastPacket' action='/Go/LastPacket'/>\n"
"      <menuitem name='PreviousPacketInConversation' action='/Go/PreviousPacketInConversation'/>\n"
"      <menuitem name='NextPacketInConversation' action='/Go/NextPacketInConversation'/>\n"
"    </menu>\n"
#ifdef HAVE_LIBPCAP
"    <menu name= 'CaptureMenu' action='/Capture'>\n"
"      <menuitem name='Interfaces' action='/Capture/Interfaces'/>\n"
"      <menuitem name='Options' action='/Capture/Options'/>\n"
"      <menuitem name='Start' action='/Capture/Start'/>\n"
"      <menuitem name='Stop' action='/Capture/Stop'/>\n"
"      <menuitem name='Restart' action='/Capture/Restart'/>\n"
"      <menuitem name='CaptureFilters' action='/Capture/CaptureFilters'/>\n"
"      <menuitem name='RefreshInterfaces' action='/Capture/RefreshInterfaces'/>\n"
"    </menu>\n"
#endif
"    <menu name= 'AnalyzeMenu' action='/Analyze'>\n"
"      <menuitem name='DisplayFilters' action='/Analyze/DisplayFilters'/>\n"
"      <menuitem name='DisplayFilterMacros' action='/Analyze/DisplayFilterMacros'/>\n"
"      <separator/>\n"
"      <menuitem name='ApplyasColumn' action='/Analyze/ApplyasColumn'/>\n"
"      <menu name= 'ApplyAsFilter' action='/ApplyasFilter'>\n"
"        <menuitem name='Selected' action='/ApplyasFilter/Selected'/>\n"
"        <menuitem name='NotSelected' action='/ApplyasFilter/Not Selected'/>\n"
"        <menuitem name='AndSelected' action='/ApplyasFilter/AndSelected'/>\n"
"        <menuitem name='OrSelected' action='/ApplyasFilter/OrSelected'/>\n"
"        <menuitem name='AndNotSelected' action='/ApplyasFilter/AndNotSelected'/>\n"
"        <menuitem name='OrNotSelected' action='/ApplyasFilter/OrNotSelected'/>\n"
"      </menu>\n"
"      <menu name= 'PrepareaFilter' action='/PrepareaFilter'>\n"
"        <menuitem name='Selected' action='/PrepareaFilter/Selected'/>\n"
"        <menuitem name='NotSelected' action='/PrepareaFilter/Not Selected'/>\n"
"        <menuitem name='AndSelected' action='/PrepareaFilter/AndSelected'/>\n"
"        <menuitem name='OrSelected' action='/PrepareaFilter/OrSelected'/>\n"
"        <menuitem name='AndNotSelected' action='/PrepareaFilter/AndNotSelected'/>\n"
"        <menuitem name='OrNotSelected' action='/PrepareaFilter/OrNotSelected'/>\n"
"      </menu>\n"
"      <separator/>\n"
"      <menuitem name='EnabledProtocols' action='/Analyze/EnabledProtocols'/>\n"
"      <menuitem name='DecodeAs' action='/Analyze/DecodeAs'/>\n"
"      <menuitem name='UserSpecifiedDecodes' action='/Analyze/UserSpecifiedDecodes'/>\n"
"      <separator/>\n"
"      <menuitem name='FollowTCPStream' action='/Analyze/FollowTCPStream'/>\n"
"      <menuitem name='FollowUDPStream' action='/Analyze/FollowUDPStream'/>\n"
"      <menuitem name='FollowSSLStream' action='/Analyze/FollowSSLStream'/>\n"
"      <menuitem name='FollowHTTPStream' action='/Analyze/FollowHTTPStream'/>\n"
"      <menuitem name='ExpertInfo' action='/Analyze/ExpertInfo'/>\n"
"      <menu name= 'ConversationFilterMenu' action='/Analyze/ConversationFilter'>\n"
"        <placeholder name='Filters'/>\n"
"      </menu>\n"
"    </menu>\n"
"    <menu name= 'StatisticsMenu' action='/Statistics'>\n"
"      <menuitem name='Summary' action='/Statistics/Summary'/>\n"
"      <menuitem name='ShowCommentsSummary' action='/Statistics/CommentsSummary'/>\n"
"      <menuitem name='ShowAddreRes' action='/Statistics/ShowAddreRes'/>\n"
"      <menuitem name='ProtocolHierarchy' action='/Statistics/ProtocolHierarchy'/>\n"
"      <menuitem name='Conversations' action='/Statistics/Conversations'/>\n"
"      <menuitem name='Endpoints' action='/Statistics/Endpoints'/>\n"
"      <menuitem name='PacketLengths' action='/Statistics/plen'/>\n"
"      <menuitem name='IOGraphs' action='/Statistics/IOGraphs'/>\n"
"      <separator/>\n"
"      <menu name= 'ConversationListMenu' action='/Statistics/ConversationList'>\n"
"        <placeholder name='Conversations'/>\n"
"      </menu>\n"
"      <menu name= 'EndpointListMenu' action='/Statistics/EndpointList'>\n"
"        <placeholder name='Endpoints'/>\n"
"      </menu>\n"
"      <menu name='ServiceResponseTimeMenu' action='/Statistics/ServiceResponseTime'>\n"
"        <menuitem name='DCE-RPC' action='/Statistics/ServiceResponseTime/DCE-RPC'/>\n"
"        <menuitem name='ONC-RPC' action='/Statistics/ServiceResponseTime/ONC-RPC'/>\n"
"      </menu>\n"
"      <separator/>\n"
"      <menu name= '29West' action='/Statistics/29West'>\n"
"        <menu name= '29WestTopicMenu' action='/Statistics/29West/Topics'>\n"
"          <menuitem name='29WestTopicAdsTopic' action='/Statistics/29West/Topics/lbmr_topic_ads_topic'/>\n"
"          <menuitem name='29WestTopicAdsSource' action='/Statistics/29West/Topics/lbmr_topic_ads_source'/>\n"
"          <menuitem name='29WestTopicAdsTransport' action='/Statistics/29West/Topics/lbmr_topic_ads_transport'/>\n"
"          <menuitem name='29WestTopicQueriesTopic' action='/Statistics/29West/Topics/lbmr_topic_queries_topic'/>\n"
"          <menuitem name='29WestTopicQueriesReceiver' action='/Statistics/29West/Topics/lbmr_topic_queries_receiver'/>\n"
"          <menuitem name='29WestTopicQueriesPattern' action='/Statistics/29West/Topics/lbmr_topic_queries_pattern'/>\n"
"          <menuitem name='29WestTopicQueriesPatternReceiver' action='/Statistics/29West/Topics/lbmr_topic_queries_pattern_receiver'/>\n"
"        </menu>\n"
"        <menu name= '29WestQueueMenu' action='/Statistics/29West/Queues'>\n"
"          <menuitem name='29WestQueueAdsQueue' action='/Statistics/29West/Queues/lbmr_queue_ads_queue'/>\n"
"          <menuitem name='29WestQueueAdsSource' action='/Statistics/29West/Queues/lbmr_queue_ads_source'/>\n"
"          <menuitem name='29WestQueueQueriesQueue' action='/Statistics/29West/Queues/lbmr_queue_queries_queue'/>\n"
"          <menuitem name='29WestQueueQueriesReceiver' action='/Statistics/29West/Queues/lbmr_queue_queries_receiver'/>\n"
"        </menu>\n"
"        <menu name= '29WestUIMMenu' action='/Statistics/29West/UIM'>\n"
"          <menuitem name='29WestUIMStreams' action='/Statistics/29West/UIM/Streams' />\n"
"          <menuitem name='29WestUIMStreamFlowGraph' action='/Statistics/29West/UIM/StreamFlowGraph' />\n"
"        </menu>\n"
"      </menu>\n"
"      <menuitem name='ANCP' action='/Statistics/ancp'/>\n"
"      <menu name= 'BACnetMenu' action='/Statistics/BACnet'>\n"
"        <menuitem name='bacapp_instanceid' action='/Statistics/BACnet/bacapp_instanceid'/>\n"
"        <menuitem name='bacapp_ip' action='/Statistics/BACnet/bacapp_ip'/>\n"
"        <menuitem name='bacapp_objectid' action='/Statistics/BACnet/bacapp_objectid'/>\n"
"        <menuitem name='bacapp_service' action='/Statistics/BACnet/bacapp_service'/>\n"
"      </menu>\n"
"      <menuitem name='Collectd' action='/Statistics/collectd'/>\n"
"      <menuitem name='Compare' action='/Statistics/compare'/>\n"
"      <menuitem name= 'DNS' action='/Statistics/dns'/>\n"
"      <menuitem name='FlowGraph' action='/Statistics/FlowGraph'/>\n"
"      <menuitem name='HART-IP' action='/Statistics/hart_ip'/>\n"
"      <menuitem name= 'Hpfeeds' action='/Statistics/hpfeeds'/>\n"
"      <menu name= 'HTTPMenu' action='/Statistics/HTTP'>\n"
"        <menuitem name='http' action='/Statistics/HTTP/http'/>\n"
"        <menuitem name='http_req' action='/Statistics/HTTP/http_req'/>\n"
"        <menuitem name='http_srv' action='/Statistics/HTTP/http_srv'/>\n"
"      </menu>\n"
"      <menuitem name='HTTP2' action='/Statistics/http2'/>\n"
"      <menu name= 'SametimeMenu' action='/Statistics/Sametime'>\n"
"        <menuitem name='sametime' action='/Statistics/Sametime/sametime'/>\n"
"      </menu>\n"
"      <menu name= 'TCPStreamGraphMenu' action='/Statistics/TCPStreamGraphMenu'>\n"
"        <menuitem name='Sequence-Graph-Stevens' action='/Statistics/TCPStreamGraphMenu/Time-Sequence-Graph-Stevens'/>\n"
"        <menuitem name='Sequence-Graph-tcptrace' action='/Statistics/TCPStreamGraphMenu/Time-Sequence-Graph-tcptrace'/>\n"
"        <menuitem name='Throughput-Graph' action='/Statistics/TCPStreamGraphMenu/Throughput-Graph'/>\n"
"        <menuitem name='RTT-Graph' action='/Statistics/TCPStreamGraphMenu/RTT-Graph'/>\n"
"        <menuitem name='Window-Scaling-Graph' action='/Statistics/TCPStreamGraphMenu/Window-Scaling-Graph'/>\n"
"      </menu>\n"
"      <menuitem name='UDPMulticastStreams' action='/Statistics/UDPMulticastStreams'/>\n"
"      <menuitem name='WLANTraffic' action='/Statistics/WLANTraffic'/>\n"
"      <separator/>\n"
"    </menu>\n"
"    <menu name= 'TelephonyMenu' action='/Telephony'>\n"
"      <menu name= 'ANSImenu' action='/Telephony/ANSI'>\n"
"      </menu>\n"
"      <menu name= 'GSM' action='/Telephony/GSM'>\n"
"        <menuitem name='MAP-Summary' action='/Telephony/GSM/MAPSummary'/>\n"
"      </menu>\n"
"      <menu name= 'IAX2menu' action='/Telephony/IAX2'>\n"
"        <menuitem name='StreamAnalysis' action='/Telephony/IAX2/StreamAnalysis'/>\n"
"      </menu>\n"
"      <menuitem name='ISUP' action='/Telephony/isup_msg'/>\n"
"      <menu name= 'LTEmenu' action='/Telephony/LTE'>\n"
"        <menuitem name='LTE_RLC_Graph' action='/Telephony/LTE/RLCGraph'/>\n"
"      </menu>\n"
"      <menu name= 'MTP3menu' action='/Telephony/MTP3'>\n"
"        <menuitem name='MSUSummary' action='/Telephony/MTP3/MSUSummary'/>\n"
"      </menu>\n"
"      <menu name= 'RTPmenu' action='/Telephony/RTP'>\n"
"        <menuitem name='ShowAllStreams' action='/Telephony/RTP/ShowAllStreams'/>\n"
"        <menuitem name='StreamAnalysis' action='/Telephony/RTP/StreamAnalysis'/>\n"
"      </menu>\n"
"      <menu name= 'RTSPmenu' action='/Telephony/RTSP'>\n"
"        <menuitem name='rtsp' action='/Telephony/RTSP/rtsp'/>\n"
"      </menu>\n"
"      <menu name= 'SCTPmenu' action='/Telephony/SCTP'>\n"
"        <menuitem name='AnalysethisAssociation' action='/Telephony/SCTP/AnalysethisAssociation'/>\n"
"        <menuitem name='ShowAllAssociations' action='/Telephony/SCTP/ShowAllAssociations'/>\n"
"      </menu>\n"
"      <menuitem name='SMPP' action='/Telephony/smpp_commands'/>\n"
"      <menuitem name='UCP' action='/Telephony/ucp_messages'/>\n"
"      <menuitem name='VoIPCalls' action='/Telephony/VoIPCalls'/>\n"
"      <menuitem name='VoIPFlows' action='/Telephony/VoIPFlows'/>\n"
"    </menu>\n"
"    <menu name= 'ToolsMenu' action='/Tools'>\n"
"      <menuitem name='FirewallACLRules' action='/Tools/FirewallACLRules'/>\n"
"    </menu>\n"
"    <menu name= 'InternalsMenu' action='/Internals'>\n"
"      <menuitem name='Dissectortables' action='/Internals/Dissectortables'/>\n"
"      <menuitem name='Conversations' action='/Internals/Conversations'/>\n"
"      <menuitem name='SupportedProtocols' action='/Internals/SupportedProtocols'/>\n"
"    </menu>\n"
"    <menu name= 'HelpMenu' action='/Help'>\n"
"      <menuitem name='Contents' action='/Help/Contents'/>\n"
"      <menu name= 'ManualPages' action='/Help/ManualPages'>\n"
"        <menuitem name='Wireshark' action='/Help/ManualPages/Wireshark'/>\n"
"        <menuitem name='WiresharkFilter' action='/Help/ManualPages/WiresharkFilter'/>\n"
"        <separator/>\n"
"        <menuitem name='Capinfos' action='/Help/ManualPages/Capinfos'/>\n"
"        <menuitem name='Dumpcap' action='/Help/ManualPages/Dumpcap'/>\n"
"        <menuitem name='Editcap' action='/Help/ManualPages/Editcap'/>\n"
"        <menuitem name='Mergecap' action='/Help/ManualPages/Mergecap'/>\n"
"        <menuitem name='RawShark' action='/Help/ManualPages/RawShark'/>\n"
"        <menuitem name='Reordercap' action='/Help/ManualPages/Reordercap'/>\n"
"        <menuitem name='Text2pcap' action='/Help/ManualPages/Text2pcap'/>\n"
"        <menuitem name='TShark' action='/Help/ManualPages/TShark'/>\n"
"      </menu>\n"
"      <separator/>\n"
"      <menuitem name='Website' action='/Help/Website'/>\n"
"      <menuitem name='FAQs' action='/Help/FAQs'/>\n"
"      <menuitem name='Ask' action='/Help/ASK'/>\n"
"      <menuitem name='Downloads' action='/Help/Downloads'/>\n"
"      <separator/>\n"
"      <menuitem name='Wiki' action='/Help/Wiki'/>\n"
"      <menuitem name='SampleCaptures' action='/Help/SampleCaptures'/>\n"
#ifdef HAVE_SOFTWARE_UPDATE
"      <separator/>\n"
"      <menuitem name='CheckForUpdates' action='/Help/CheckForUpdates'/>\n"
#endif /* HAVE_SOFTWARE_UPDATE */
"      <separator/>\n"
"      <menuitem name='AboutWireshark' action='/Help/AboutWireshark'/>\n"
"    </menu>\n"
"  </menubar>\n"
"</ui>\n";



/*
 * Main menu.
 *
 * Please do not use keystrokes that are used as "universal" shortcuts in
 * various desktop environments:
 *
 *   Windows:
 *  http://support.microsoft.com/kb/126449
 *
 *   GNOME:
 *  http://library.gnome.org/users/user-guide/nightly/keyboard-skills.html.en
 *
 *   KDE:
 *  http://developer.kde.org/documentation/standards/kde/style/keys/shortcuts.html
 *
 * In particular, do not use the following <control> sequences for anything
 * other than their standard purposes:
 *
 *  <control>O  File->Open
 *  <control>S  File->Save
 *  <control>P  File->Print
 *  <control>W  File->Close
 *  <control>Q  File->Quit
 *  <control>Z  Edit->Undo (which we don't currently have)
 *  <control>X  Edit->Cut (which we don't currently have)
 *  <control>C  Edit->Copy (which we don't currently have)
 *  <control>V  Edit->Paste (which we don't currently have)
 *  <control>A  Edit->Select All (which we don't currently have)
 *
 * Note that some if not all of the Edit keys above already perform those
 * functions in text boxes, such as the Filter box.  Do no, under any
 * circumstances, make a change that keeps them from doing so.
 */

/*
 * GtkActionEntry
 * typedef struct {
 *   const gchar     *name;
 *   const gchar     *stock_id;
 *   const gchar     *label;
 *   const gchar     *accelerator;
 *   const gchar     *tooltip;
 *   GCallback  callback;
 * } GtkActionEntry;
 * const gchar *name;           The name of the action.
 * const gchar *stock_id;       The stock id for the action, or the name of an icon from the icon theme.
 * const gchar *label;          The label for the action. This field should typically be marked for translation,
 *                              see gtk_action_group_set_translation_domain().
 *                              If label is NULL, the label of the stock item with id stock_id is used.
 * const gchar *accelerator;    The accelerator for the action, in the format understood by gtk_accelerator_parse().
 * const gchar *tooltip;        The tooltip for the action. This field should typically be marked for translation,
 *                              see gtk_action_group_set_translation_domain().
 * GCallback callback;          The function to call when the action is activated.
 *
 */

#ifdef HAVE_LIBPCAP
/*
 * TODO Move this menu to capture_if_dlg.c
 * eg put a "place holder" in the UI description and
 * make a call from main_menubar.c i.e build_capture_menu()
 * ad do the UI stuff there.
 */
static const GtkActionEntry capture_menu_entries[] = {
   { "/Capture",                    NULL,                               "_Capture",             NULL,            NULL,    NULL },
   { "/Capture/Interfaces",         WIRESHARK_STOCK_CAPTURE_INTERFACES, "_Interfaces...",       "<control>I",    NULL,    G_CALLBACK(capture_if_action_cb) },
   { "/Capture/Options",            WIRESHARK_STOCK_CAPTURE_OPTIONS,    "_Options...",          "<control>K",    NULL,    G_CALLBACK(capture_prep_action_cb) },
   { "/Capture/Start",              WIRESHARK_STOCK_CAPTURE_START,      "_Start",               "<control>E",    NULL,    G_CALLBACK(capture_start_action_cb) },
   { "/Capture/Stop",               WIRESHARK_STOCK_CAPTURE_STOP,       "S_top",                "<control>E",    NULL,    G_CALLBACK(capture_stop_action_cb) },
   { "/Capture/Restart",            WIRESHARK_STOCK_CAPTURE_RESTART,    "_Restart",             "<control>R",    NULL,    G_CALLBACK(capture_restart_action_cb) },
   { "/Capture/CaptureFilters",     WIRESHARK_STOCK_CAPTURE_FILTER,     "Capture _Filters...",  NULL,            NULL,    G_CALLBACK(capture_filters_action_cb) },
   { "/Capture/RefreshInterfaces",  GTK_STOCK_REFRESH,                  "Refresh Interfaces",   NULL,            NULL,    G_CALLBACK(refresh_interfaces_action_cb) },
};
#endif

static const GtkActionEntry main_menu_bar_entries[] = {
  /* Top level */
  { "/File",                    NULL,                              "_File",              NULL,                   NULL,           NULL },
  { "/Edit",                    NULL,                              "_Edit",              NULL,                   NULL,           NULL },
  { "/View",                    NULL,                              "_View",              NULL,                   NULL,           NULL },
  { "/Go",                      NULL,                              "_Go",                NULL,                   NULL,           NULL },
  { "/Analyze",                 NULL,                              "_Analyze",           NULL,                   NULL,           NULL },
  { "/Statistics",              NULL,                              "_Statistics",        NULL,                   NULL,           NULL },
  { "/Telephony",               NULL,                              "Telephon_y",         NULL,                   NULL,           NULL },
  { "/Tools",                   NULL,                              "_Tools",             NULL,                   NULL,           NULL },
  { "/Internals",               NULL,                              "_Internals",         NULL,                   NULL,           NULL },
  { "/Help",                    NULL,                              "_Help",              NULL,                   NULL,           NULL },

  { "/File/Open",               GTK_STOCK_OPEN,                    "_Open...",           "<control>O",           "Open a file",  G_CALLBACK(file_open_cmd_cb) },
  { "/File/OpenRecent",         NULL,                              "Open _Recent",       NULL,                   NULL,           NULL },
  { "/File/Merge",              NULL,                              "_Merge...",          NULL,                   NULL,           G_CALLBACK(file_merge_cmd_cb) },
  { "/File/ImportFromHexDump",  NULL,                              "_Import from Hex Dump...", NULL,                   NULL,           G_CALLBACK(file_import_cmd_cb) },
  { "/File/Close",              GTK_STOCK_CLOSE,                   "_Close",             "<control>W",           NULL,           G_CALLBACK(file_close_cmd_cb) },

  { "/File/Save",               WIRESHARK_STOCK_SAVE,              "_Save",              "<control>S",           NULL,           G_CALLBACK(file_save_cmd_cb) },
  { "/File/SaveAs",             WIRESHARK_STOCK_SAVE,              "Save _As...",        "<shift><control>S",    NULL,           G_CALLBACK(file_save_as_cmd_cb) },

  { "/File/Set",                NULL,                              "File Set",           NULL,                   NULL,           NULL },
  { "/File/ExportSpecifiedPackets", NULL,         "Export Specified Packets...", NULL,           NULL,           G_CALLBACK(file_export_specified_packets_cmd_cb) },
  { "/File/ExportPacketDissections",  NULL,                        "Export Packet Dissections", NULL,                   NULL,           NULL },
  { "/File/ExportSelectedPacketBytes", NULL,         "Export Selected Packet _Bytes...", "<control>H",           NULL,           G_CALLBACK(savehex_cb) },
  { "/File/ExportPDUs",            NULL,                           "Export PDUs to File...",  NULL,                NULL,           G_CALLBACK(export_pdu_show_cb) },
  { "/File/ExportSSLSessionKeys",  NULL,                   "Export SSL Session Keys...", NULL,                   NULL,           G_CALLBACK(savesslkeys_cb) },
  { "/File/ExportObjects",      NULL,                              "Export Objects",     NULL,                   NULL,           NULL },
  { "/File/Print",              GTK_STOCK_PRINT,                   "_Print...",          "<control>P",           NULL,           G_CALLBACK(file_print_cmd_cb) },
  { "/File/Quit",               GTK_STOCK_QUIT,                    "_Quit",              "<control>Q",           NULL,           G_CALLBACK(file_quit_cmd_cb) },

  { "/File/Set/ListFiles",      WIRESHARK_STOCK_FILE_SET_LIST,     "List Files",         NULL,                   NULL,           G_CALLBACK(fileset_cb) },
  { "/File/Set/NextFile",       WIRESHARK_STOCK_FILE_SET_NEXT,     "Next File",          NULL,                   NULL,           G_CALLBACK(fileset_next_cb) },
  { "/File/Set/PreviousFile",   WIRESHARK_STOCK_FILE_SET_PREVIOUS, "Previous File",      NULL,                   NULL,           G_CALLBACK(fileset_previous_cb) },

  { "/File/ExportPacketDissections/Text",       NULL,       "as \"Plain _Text\" file...",      NULL,                   NULL,           G_CALLBACK(export_text_cmd_cb) },
  { "/File/ExportPacketDissections/PostScript", NULL,       "as \"_PostScript\" file...",      NULL,                   NULL,           G_CALLBACK(export_ps_cmd_cb) },
  { "/File/ExportPacketDissections/CSV",        NULL,       "as \"_CSV\" (Comma Separated Values packet summary) file...",
                                                                                         NULL,                   NULL,           G_CALLBACK(export_csv_cmd_cb) },
  { "/File/ExportPacketDissections/CArrays",    NULL,       "as \"C _Arrays\" (packet bytes) file...",
                                                                                         NULL,                   NULL,           G_CALLBACK(export_carrays_cmd_cb) },
  { "/File/ExportPacketDissections/PSML",       NULL,       "as XML - \"P_SML\" (packet summary) file...",
                                                                                         NULL,                   NULL,           G_CALLBACK(export_psml_cmd_cb) },
  { "/File/ExportPacketDissections/PDML",       NULL,       "as XML - \"P_DML\" (packet details) file...",
                                                                                         NULL,                   NULL,           G_CALLBACK(export_pdml_cmd_cb) },
  { "/File/ExportPacketDissections/JSON",       NULL,       "as \"_JSON\" file...",
                                                                                         NULL,                   NULL,           G_CALLBACK(export_json_cmd_cb) },
  { "/File/ExportObjects/HTTP",           NULL,       "_HTTP",                           NULL,                   NULL,           G_CALLBACK(eo_http_cb) },
  { "/File/ExportObjects/DICOM",          NULL,       "_DICOM",                          NULL,                   NULL,           G_CALLBACK(eo_dicom_cb) },
  { "/File/ExportObjects/SMB",            NULL,       "_SMB/SMB2",                            NULL,                   NULL,           G_CALLBACK(eo_smb_cb) },
  { "/File/ExportObjects/TFTP",            NULL,       "_TFTP",                          NULL,                   NULL,           G_CALLBACK(eo_tftp_cb) },

  { "/Edit/Copy",                         NULL,       "Copy",                            NULL,                   NULL,           NULL },

  { "/Edit/Copy/Description",             NULL,       "Description",                     "<shift><control>D",    NULL,           G_CALLBACK(copy_description_cb) },
  { "/Edit/Copy/Fieldname",               NULL,       "Fieldname",                       "<shift><control>F",    NULL,           G_CALLBACK(copy_fieldname_cb) },
  { "/Edit/Copy/Value",                   NULL,       "Value",                           "<shift><control>V",    NULL,           G_CALLBACK(copy_value_cb) },
  { "/Edit/Copy/AsFilter",                NULL,       "As Filter",                       "<shift><control>C",    NULL,           G_CALLBACK(copy_as_filter_cb) },

#if 0
    /*
     * Un-#if this when we actually implement Cut/Copy/Paste for the
     * packet list and packet detail windows.
     *
     * Note: when we implement Cut/Copy/Paste in those windows, we
     * will almost certainly want to allow multiple packets to be
     * selected in the packet list pane and multiple packet detail
     * items to be selected in the packet detail pane, so that
     * the user can, for example, copy the summaries of multiple
     * packets to the clipboard from the packet list pane and multiple
     * packet detail items - perhaps *all* packet detail items - from
     * the packet detail pane.  Given that, we'll also want to
     * implement Select All.
     *
     * If multiple packets are selected, we would probably display nothing
     * in the packet detail pane, just as we do if no packet is selected,
     * and any menu items etc. that would pertain only to a single packet
     * would be disabled.
     *
     * If multiple packet detail items are selected, we would probably
     * disable all items that pertain only to a single packet detail
     * item, such as some items in the status bar.
     *
     * XXX - the actions for these will be different depending on what
     * widget we're in; ^C should copy from the filter text widget if
     * we're in that widget, the packet list if we're in that widget
     * (presumably copying the summaries of selected packets to the
     * clipboard, e.g. the text copy would be the text of the columns),
     * the packet detail if we're in that widget (presumably copying
     * the contents of selected protocol tree items to the clipboard,
     * e.g. the text copy would be the text displayed for those items),
     * etc..
     *
     * Given that those menu items should also affect text widgets
     * such as the filter box, we would again want Select All, and,
     * at least for the filter box, we would also want Undo and Redo.
     * We would only want Cut, Paste, Undo, and Redo for the packet
     * list and packet detail panes if we support modifying them.
     */
    {"/Edit/_Undo", "<control>Z", NULL,
                             0, "<StockItem>", GTK_STOCK_UNDO,},
    {"/Edit/_Redo", "<shift><control>Z", NULL,
                             0, "<StockItem>", GTK_STOCK_REDO,},
    {"/Edit/<separator>", NULL, NULL, 0, "<Separator>", NULL,},
    {"/Edit/Cu_t", "<control>X", NULL,
                             0, "<StockItem>", GTK_STOCK_CUT,},
    {"/Edit/_Copy", "<control>C", NULL,
                             0, "<StockItem>", GTK_STOCK_COPY,},
    {"/Edit/_Paste", "<control>V", NULL,
                             0, "<StockItem>", GTK_STOCK_PASTE,},
    {"/Edit/<separator>", NULL, NULL, 0, "<Separator>", NULL,},
    {"/Edit/Select _All", "<control>A", NULL, 0,
                             "<StockItem>", GTK_STOCK_SELECT_ALL,},
#endif /* 0 */
   { "/Edit/FindPacket",                GTK_STOCK_FIND,     "_Find Packet...",                      "<control>F",           NULL,           G_CALLBACK(find_frame_cb) },
   { "/Edit/FindNext",                  NULL,               "Find Ne_xt",                           "<control>N",           NULL,           G_CALLBACK(find_next_cb) },
   { "/Edit/FindPrevious",              NULL,               "Find Pre_vious",                       "<control>B",           NULL,           G_CALLBACK(find_previous_cb) },

   { "/Edit/MarkPacket",                NULL,               "_Mark/Unmark Packet",                  "<control>M",           NULL,           G_CALLBACK(packet_list_mark_frame_cb) },
   /* XXX - Unused. Should this and its associated code be removed? */
   { "/Edit/ToggleMarkingOfAllDisplayedPackets",    NULL,   "Toggle Marking of All Displayed Packets",  "<shift><alt><control>M",           NULL,           G_CALLBACK(packet_list_toggle_mark_all_displayed_frames_cb) },
   { "/Edit/MarkAllDisplayedPackets",   NULL,               "Mark All Displayed Packets",           "<shift><control>M",    NULL,           G_CALLBACK(packet_list_mark_all_displayed_frames_cb) },
   { "/Edit/UnmarkAllDisplayedPackets", NULL,               "_Unmark All Displayed Packets",        "<alt><control>M",      NULL,           G_CALLBACK(packet_list_unmark_all_displayed_frames_cb) },
   { "/Edit/FindNextMark",              NULL,               "Next Mark",                            "<shift><control>N",    NULL,           G_CALLBACK(find_next_mark_cb) },
   { "/Edit/FindPreviousMark",          NULL,               "Previous Mark",                        "<shift><control>B",    NULL,           G_CALLBACK(find_prev_mark_cb) },

   { "/Edit/IgnorePacket",              NULL,               "_Ignore/Unignore Packet",              "<control>D",           NULL,           G_CALLBACK(packet_list_ignore_frame_cb) },
    /*
     * XXX - this next one overrides /Edit/Copy/Description
     */
   { "/Edit/IgnoreAllDisplayedPackets", NULL,               "Ignore All Displayed Packets",         "<shift><control>D",        NULL,           G_CALLBACK(packet_list_ignore_all_displayed_frames_cb) },
   { "/Edit/Un-IgnoreAllPackets",       NULL,               "U_nignore All Packets",                "<alt><control>D",          NULL,           G_CALLBACK(packet_list_unignore_all_frames_cb) },
   { "/Edit/SetTimeReference",          WIRESHARK_STOCK_TIME,   "Set/Unset Time Reference",         "<control>T",           NULL,           G_CALLBACK(set_reftime_cb) },
   { "/Edit/Un-TimeReferenceAllPackets",NULL,               "Unset All Time References",            "<alt><control>T",          NULL,           G_CALLBACK(packet_list_untime_reference_all_frames_cb) },
   { "/Edit/FindNextTimeReference",     NULL,               "Next Time Reference",                  "<alt><control>N",          NULL,           G_CALLBACK(find_next_ref_time_cb) },
   { "/Edit/FindPreviousTimeReference", NULL,               "Previous Time Reference",              "<alt><control>B",          NULL,           G_CALLBACK(find_previous_ref_time_cb) },
   { "/Edit/TimeShift",             WIRESHARK_STOCK_TIME,   "Time Shift...",                        "<shift><control>T",        NULL,           G_CALLBACK(time_shift_cb) },

   { "/Edit/ConfigurationProfiles", NULL,                   "_Configuration Profiles...",           "<shift><control>A",        NULL,           G_CALLBACK(profile_dialog_cb) },
   { "/Edit/Preferences",           GTK_STOCK_PREFERENCES,  "_Preferences...",                      "<shift><control>P",        NULL,           G_CALLBACK(menus_prefs_cb) },
#ifdef WANT_PACKET_EDITOR
   { "/Edit/EditPacket",                NULL,               "_Edit Packet",                          NULL,                      NULL,           G_CALLBACK(edit_window_cb) },
#endif
   { "/Edit/AddEditPktComment",         WIRESHARK_STOCK_EDIT,   "Packet Comment...",                "<alt><control>C",          NULL,           G_CALLBACK(edit_packet_comment_dlg) },
   { "/Edit/AddEditCaptureComment",     NULL,                   "Capture Comment...",               "<shift><alt><control>C",   NULL,           G_CALLBACK(edit_capture_comment_dlg_launch) },

   { "/View/TimeDisplayFormat",     NULL,                   "_Time Display Format",                  NULL,                      NULL,           NULL },

   { "/View/NameResolution",                    NULL,        "Name Resol_ution",                     NULL,                      NULL,           NULL },
   { "/View/NameResolution/ResolveName",        NULL,        "_Resolve Name",                        NULL,                      NULL,           G_CALLBACK(resolve_name_cb) },
   { "/View/NameResolution/ManuallyResolveName",NULL,        "Manually Resolve Name",                NULL,                      NULL,           G_CALLBACK(manual_addr_resolv_dlg) },

   { "/View/ZoomIn",                GTK_STOCK_ZOOM_IN,      "_Zoom In",                             "<control>plus",            NULL,           G_CALLBACK(view_zoom_in_cb) },
   { "/View/ZoomOut",               GTK_STOCK_ZOOM_OUT,     "Zoom _Out",                            "<control>minus",           NULL,           G_CALLBACK(view_zoom_out_cb) },
   { "/View/NormalSize",            GTK_STOCK_ZOOM_100,     "_Normal Size",                         "<control>equal",           NULL,           G_CALLBACK(view_zoom_100_cb) },
   { "/View/ResizeAllColumns",      WIRESHARK_STOCK_RESIZE_COLUMNS, "Resize All Columns",           "<shift><control>R",        NULL,           G_CALLBACK(packet_list_resize_columns_cb) },
   { "/View/DisplayedColumns",      NULL,                   "Displayed Columns",                    NULL,                       NULL,           NULL },
   { "/View/ExpandSubtrees",        NULL,                   "E_xpand Subtrees",                      "<shift>Right",             NULL,           G_CALLBACK(expand_tree_cb) },
   { "/View/CollapseSubtrees",      NULL,                   "Collapse Subtrees",                     "<shift>Left",              NULL,           G_CALLBACK(collapse_tree_cb) },
   { "/View/ExpandAll",             NULL,                   "_Expand All",                           "<control>Right",           NULL,           G_CALLBACK(expand_all_cb) },
   { "/View/CollapseAll",           NULL,                   "Collapse _All",                         "<control>Left",            NULL,           G_CALLBACK(collapse_all_cb) },
   { "/View/ColorizeConversation",  NULL,                   "Colorize Conversation",NULL,                   NULL,           NULL },

   { "/View/ColorizeConversation/Color 1",  WIRESHARK_STOCK_COLOR1, "Color 1",                  "<control>1", NULL, G_CALLBACK(view_menu_color_conv_color1_cb) },
   { "/View/ColorizeConversation/Color 2",  WIRESHARK_STOCK_COLOR2, "Color 2",                  "<control>2", NULL, G_CALLBACK(view_menu_color_conv_color2_cb) },
   { "/View/ColorizeConversation/Color 3",  WIRESHARK_STOCK_COLOR3, "Color 3",                  "<control>3", NULL, G_CALLBACK(view_menu_color_conv_color3_cb) },
   { "/View/ColorizeConversation/Color 4",  WIRESHARK_STOCK_COLOR4, "Color 4",                  "<control>4", NULL, G_CALLBACK(view_menu_color_conv_color4_cb) },
   { "/View/ColorizeConversation/Color 5",  WIRESHARK_STOCK_COLOR5, "Color 5",                  "<control>5", NULL, G_CALLBACK(view_menu_color_conv_color5_cb) },
   { "/View/ColorizeConversation/Color 6",  WIRESHARK_STOCK_COLOR6, "Color 6",                  "<control>6", NULL, G_CALLBACK(view_menu_color_conv_color6_cb) },
   { "/View/ColorizeConversation/Color 7",  WIRESHARK_STOCK_COLOR7, "Color 7",                  "<control>7", NULL, G_CALLBACK(view_menu_color_conv_color7_cb) },
   { "/View/ColorizeConversation/Color 8",  WIRESHARK_STOCK_COLOR8, "Color 8",                  "<control>8", NULL, G_CALLBACK(view_menu_color_conv_color8_cb) },
   { "/View/ColorizeConversation/Color 9",  WIRESHARK_STOCK_COLOR9, "Color 9",                  "<control>9", NULL, G_CALLBACK(view_menu_color_conv_color9_cb) },
   { "/View/ColorizeConversation/Color 10", WIRESHARK_STOCK_COLOR0, "Color 10",                 "<control>0", NULL, G_CALLBACK(view_menu_color_conv_color10_cb) },
   { "/View/ColorizeConversation/NewColoringRule",  NULL,           "New Coloring Rule...",     NULL,         NULL, G_CALLBACK(view_menu_color_conv_new_rule_cb) },

   { "/View/ResetColoring1-10",     NULL,                   "Reset Coloring 1-10",              "<control>space",               NULL,               G_CALLBACK(view_menu_reset_coloring_cb) },
   { "/View/ColoringRules",         GTK_STOCK_SELECT_COLOR, "_Coloring Rules...",               NULL,                           NULL,               G_CALLBACK(color_display_cb) },
   { "/View/ShowPacketinNewWindow", NULL,                   "Show Packet in New _Window",       NULL,                           NULL,               G_CALLBACK(new_window_cb) },
   { "/View/Reload",                GTK_STOCK_REFRESH,      "_Reload",                          "<control>R",                   NULL,               G_CALLBACK(file_reload_cmd_cb) },


   { "/Go/Back",                    GTK_STOCK_GO_BACK,      "_Back",                            "<alt>Left",                    NULL,               G_CALLBACK(history_back_cb) },
   { "/Go/Forward",                 GTK_STOCK_GO_FORWARD,   "_Forward",                         "<alt>Right",                   NULL,               G_CALLBACK(history_forward_cb) },
   { "/Go/Goto",                    GTK_STOCK_JUMP_TO,      "_Go to Packet...",                 "<control>G",                   NULL,               G_CALLBACK(goto_frame_cb) },
   { "/Go/GotoCorrespondingPacket", NULL,                   "Go to _Corresponding Packet",      NULL,                           NULL,               G_CALLBACK(goto_framenum_cb) },
   { "/Go/PreviousPacket",          GTK_STOCK_GO_UP,        "Previous Packet",                  "<control>Up",                  NULL,               G_CALLBACK(goto_previous_frame_cb) },
   { "/Go/NextPacket",              GTK_STOCK_GO_DOWN,      "Next Packet",                      "<control>Down",                NULL,               G_CALLBACK(goto_next_frame_cb) },
   { "/Go/FirstPacket",             GTK_STOCK_GOTO_TOP,     "F_irst Packet",                    "<control>Home",                NULL,               G_CALLBACK(goto_top_frame_cb) },
   { "/Go/LastPacket",              GTK_STOCK_GOTO_BOTTOM,  "_Last Packet",                     "<control>End",                 NULL,               G_CALLBACK(goto_bottom_frame_cb) },
   { "/Go/PreviousPacketInConversation",            GTK_STOCK_GO_UP,        "Previous Packet In Conversation",                  "<control>comma",                   NULL,               G_CALLBACK(goto_previous_frame_conversation_cb) },
   { "/Go/NextPacketInConversation",                GTK_STOCK_GO_DOWN,      "Next Packet In Conversation",                      "<control>period",              NULL,               G_CALLBACK(goto_next_frame_conversation_cb) },


   { "/Analyze/DisplayFilters",     WIRESHARK_STOCK_DISPLAY_FILTER,     "_Display Filters...",  NULL,                           NULL,               G_CALLBACK(dfilter_dialog_cb) },

   { "/Analyze/DisplayFilterMacros",            NULL,                   "Display Filter _Macros...",    NULL,                   NULL,               G_CALLBACK(macros_dialog_cb) },
   { "/Analyze/ApplyasColumn",                  NULL,                           "Apply as Column",      NULL,                   NULL,               G_CALLBACK(apply_as_custom_column_cb) },

   { "/Analyze/EnabledProtocols",   WIRESHARK_STOCK_CHECKBOX, "_Enabled Protocols...",  "<shift><control>E", NULL, G_CALLBACK(proto_cb) },
   { "/Analyze/DecodeAs",   WIRESHARK_STOCK_DECODE_AS, "Decode _As...",         NULL, NULL, G_CALLBACK(decode_as_cb) },
   { "/Analyze/UserSpecifiedDecodes",   WIRESHARK_STOCK_DECODE_AS, "_User Specified Decodes...",            NULL, NULL, G_CALLBACK(decode_show_cb) },

   { "/Analyze/FollowTCPStream",                            NULL,       "Follow TCP Stream",                    NULL, NULL, G_CALLBACK(follow_tcp_stream_cb) },
   { "/Analyze/FollowUDPStream",                            NULL,       "Follow UDP Stream",                    NULL, NULL, G_CALLBACK(follow_udp_stream_cb) },
   { "/Analyze/FollowSSLStream",                            NULL,       "Follow SSL Stream",                    NULL, NULL, G_CALLBACK(follow_ssl_stream_cb) },
   { "/Analyze/FollowHTTPStream",                           NULL,       "Follow HTTP Stream",                   NULL, NULL, G_CALLBACK(follow_http_stream_cb) },

   { "/Analyze/ExpertInfo",          WIRESHARK_STOCK_EXPERT_INFO,       "Expert _Info",               NULL, NULL, G_CALLBACK(expert_comp_dlg_launch) },

   { "/Analyze/ConversationFilter",                         NULL,       "Conversation Filter",                  NULL, NULL, NULL },


   { "/Statistics/ConversationList",                           NULL,       "_Conversation List",                   NULL, NULL, NULL },

   { "/Statistics/EndpointList",                                NULL,               "_Endpoint List",               NULL, NULL, NULL },

   { "/Statistics/ServiceResponseTime",             NULL,           "Service _Response Time",       NULL, NULL, NULL },
   { "/Statistics/ServiceResponseTime/DCE-RPC",     WIRESHARK_STOCK_TIME,           "DCE-RPC...",                   NULL, NULL, G_CALLBACK(gtk_dcerpcstat_cb) },
   { "/Statistics/ServiceResponseTime/ONC-RPC",     WIRESHARK_STOCK_TIME,           "ONC-RPC...",                   NULL, NULL, G_CALLBACK(gtk_rpcstat_cb) },

   { "/Statistics/29West",                                            NULL,                       "29West",                       NULL, NULL, NULL },
   { "/Statistics/29West/Topics",                                     NULL,                       "Topics",                       NULL, NULL, NULL },
   { "/Statistics/29West/Topics/lbmr_topic_ads_topic",                NULL,                       "Advertisements by Topic",      NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/29West/Topics/lbmr_topic_ads_source",               NULL,                       "Advertisements by Source",     NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/29West/Topics/lbmr_topic_ads_transport",            NULL,                       "Advertisements by Transport",  NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/29West/Topics/lbmr_topic_queries_topic",            NULL,                       "Queries by Topic",             NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/29West/Topics/lbmr_topic_queries_receiver",         NULL,                       "Queries by Receiver",          NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/29West/Topics/lbmr_topic_queries_pattern",          NULL,                       "Wildcard Queries by Pattern",  NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/29West/Topics/lbmr_topic_queries_pattern_receiver", NULL,                       "Wildcard Queries by Receiver", NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/29West/Queues",                                     NULL,                       "Queues",                       NULL, NULL, NULL },
   { "/Statistics/29West/Queues/lbmr_queue_ads_queue",                NULL,                       "Advertisements by Queue",      NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/29West/Queues/lbmr_queue_ads_source",               NULL,                       "Advertisements by Source",     NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/29West/Queues/lbmr_queue_queries_queue",            NULL,                       "Queries by Queue",             NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/29West/Queues/lbmr_queue_queries_receiver",         NULL,                       "Queries by Receiver",          NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/29West/UIM",                                        NULL,                       "UIM",                          NULL, NULL, NULL },
   { "/Statistics/29West/UIM/Streams",                                NULL,                       "Streams",                      NULL, NULL, G_CALLBACK(lbmc_stream_dlg_stream_menu_cb) },
   { "/Statistics/29West/UIM/StreamFlowGraph",                        WIRESHARK_STOCK_FLOW_GRAPH, "Stream Flow Graph",            NULL, NULL, G_CALLBACK(lbmc_uim_flow_menu_cb) },

   { "/Statistics/ancp",                            NULL,       "ANCP",                             NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/BACnet",                          NULL,       "BACnet",                           NULL, NULL, NULL },
   { "/Statistics/BACnet/bacapp_instanceid",        NULL,       "Packets sorted by Instance ID",    NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/BACnet/bacapp_ip",                NULL,       "Packets sorted by IP",             NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/BACnet/bacapp_objectid",          NULL,       "Packets sorted by Object Type",    NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/BACnet/bacapp_service",           NULL,       "Packets sorted by Service",        NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/collectd",                        NULL,       "Collectd...",                      NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/compare",                         NULL,       "Compare...",                       NULL, NULL, G_CALLBACK(gtk_comparestat_cb) },
   { "/Statistics/dns",                             NULL,       "DNS",                              NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/FlowGraph",       WIRESHARK_STOCK_FLOW_GRAPH, "Flo_w Graph...",                   NULL, NULL, G_CALLBACK(flow_graph_launch) },
   { "/Statistics/hart_ip",                         NULL,       "HART-IP",                          NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/hpfeeds",                         NULL,       "HPFEEDS",                          NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/HTTP",                            NULL,       "HTTP",                             NULL, NULL, NULL },
   { "/Statistics/HTTP/http",                       NULL,       "Packet Counter",                   NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/HTTP/http_req",                   NULL,       "Requests",                         NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/HTTP/http_srv",                   NULL,       "Load Distribution",                NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/http2",                           NULL,       "HTTP2",                          NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/Sametime",                        NULL,       "Sametime",                         NULL, NULL, NULL },
   { "/Statistics/Sametime/sametime",               NULL,       "Messages",                         NULL, NULL, G_CALLBACK(gtk_stats_tree_cb) },
   { "/Statistics/TCPStreamGraphMenu",  NULL,           "TCP StreamGraph",                          NULL, NULL, NULL },
   { "/Statistics/TCPStreamGraphMenu/Time-Sequence-Graph-Stevens",  NULL, "Time-Sequence Graph (Stevens)",  NULL, NULL, G_CALLBACK(tcp_graph_cb) },
   { "/Statistics/TCPStreamGraphMenu/Time-Sequence-Graph-tcptrace", NULL, "Time-Sequence Graph (tcptrace)", NULL, NULL, G_CALLBACK(tcp_graph_cb) },
   { "/Statistics/TCPStreamGraphMenu/Throughput-Graph",             NULL, "Throughput Graph",               NULL, NULL, G_CALLBACK(tcp_graph_cb) },
   { "/Statistics/TCPStreamGraphMenu/RTT-Graph",                    NULL, "Round Trip Time Graph",          NULL, NULL, G_CALLBACK(tcp_graph_cb) },
   { "/Statistics/TCPStreamGraphMenu/Window-Scaling-Graph",         NULL, "Window Scaling Graph",           NULL, NULL, G_CALLBACK(tcp_graph_cb) },

   { "/Statistics/UDPMulticastStreams",                             NULL, "UDP Multicast Streams",          NULL, NULL, G_CALLBACK(mcaststream_launch) },
   { "/Statistics/WLANTraffic",                                     NULL, "WLAN Traffic",                   NULL, NULL, G_CALLBACK(wlanstat_launch) },

   { "/Statistics/Summary",                     GTK_STOCK_PROPERTIES,           "_Summary",                     NULL, NULL, G_CALLBACK(summary_open_cb) },
   { "/Statistics/CommentsSummary",             NULL,                           "Comments Summary",             NULL, NULL, G_CALLBACK(show_packet_comment_summary_dlg) },
   { "/Statistics/ShowAddreRes",                NULL,                           "Show address resolution",      NULL, NULL, G_CALLBACK(addr_resolution_dlg) },
   { "/Statistics/ProtocolHierarchy",           NULL,                           "_Protocol Hierarchy",          NULL, NULL, G_CALLBACK(proto_hier_stats_cb) },
   { "/Statistics/Conversations",   WIRESHARK_STOCK_CONVERSATIONS,  "Conversations",            NULL,                       NULL,               G_CALLBACK(init_conversation_notebook_cb) },
   { "/Statistics/Endpoints",       WIRESHARK_STOCK_ENDPOINTS,      "Endpoints",                NULL,                       NULL,               G_CALLBACK(init_hostlist_notebook_cb) },
   { "/Statistics/IOGraphs",            WIRESHARK_STOCK_GRAPHS,     "_IO Graph",                NULL,                       NULL,               G_CALLBACK(gui_iostat_cb) },
   { "/Statistics/plen",                        NULL,               "Packet Lengths...",        NULL,                       NULL,               G_CALLBACK(gtk_stats_tree_cb) },

   { "/Telephony/ANSI",                 NULL,                       "_ANSI",                    NULL, NULL, NULL },
   { "/Telephony/GSM",                  NULL,                       "_GSM",                     NULL, NULL, NULL },
   { "/Telephony/GSM/MAPSummary",       NULL,                       "MAP Summary",              NULL,                       NULL,               G_CALLBACK(gsm_map_stat_gtk_sum_cb) },

   { "/Telephony/IAX2",                 NULL,                       "IA_X2",                    NULL, NULL, NULL },
   { "/Telephony/IAX2/StreamAnalysis",  NULL,                       "Stream Analysis...",       NULL,                       NULL,               G_CALLBACK(iax2_analysis_cb) },

   { "/Telephony/isup_msg",             NULL,                       "_ISUP Messages",           NULL,                       NULL,               G_CALLBACK(gtk_stats_tree_cb) },

   { "/Telephony/LTE",                  NULL,                       "_LTE",                     NULL, NULL, NULL },
   { "/Telephony/LTE/RLCGraph",         NULL,                       "RLC _Graph...",            NULL,                       NULL,               G_CALLBACK(rlc_lte_graph_cb) },
   { "/Telephony/MTP3",                 NULL,                       "_MTP3",                    NULL, NULL, NULL },
   { "/Telephony/MTP3/MSUSummary",      NULL,                       "MSU Summary",              NULL,                       NULL,               G_CALLBACK(mtp3_sum_gtk_sum_cb) },
   { "/Telephony/RTP",                  NULL,                       "_RTP",                     NULL, NULL, NULL },
   { "/Telephony/RTP/StreamAnalysis",   NULL,                       "Stream Analysis...",       NULL,                       NULL,               G_CALLBACK(rtp_analysis_cb) },
   { "/Telephony/RTP/ShowAllStreams",   NULL,                       "Show All Streams",         NULL,                       NULL,               G_CALLBACK(rtpstream_launch) },
   { "/Telephony/RTSP",                 NULL,                       "RTSP",                     NULL, NULL, NULL },
   { "/Telephony/RTSP/rtsp",            NULL,                       "Packet Counter",           NULL,                       NULL,               G_CALLBACK(gtk_stats_tree_cb) },
   { "/Telephony/SCTP",                 NULL,                       "S_CTP",                        NULL, NULL, NULL },
   { "/Telephony/SCTP/AnalysethisAssociation",  NULL,               "Analyse this Association", NULL,                       NULL,               G_CALLBACK(sctp_analyse_start) },
   { "/Telephony/SCTP/ShowAllAssociations",     NULL,               "Show All Associations...", NULL,                       NULL,               G_CALLBACK(sctp_stat_start) },
   { "/Telephony/smpp_commands",        NULL,                       "SM_PPOperations",          NULL,                       NULL,               G_CALLBACK(gtk_stats_tree_cb) },
   { "/Telephony/ucp_messages",         NULL,                       "_UCP Messages",            NULL,                       NULL,               G_CALLBACK(gtk_stats_tree_cb) },
   { "/Telephony/VoIPCalls",            WIRESHARK_STOCK_TELEPHONE,  "_VoIP Calls",              NULL,                       NULL,               G_CALLBACK(voip_calls_launch) },
   { "/Telephony/VoIPFlows",            WIRESHARK_STOCK_TELEPHONE,  "SIP _Flows",               NULL,                       NULL,               G_CALLBACK(voip_flows_launch) },

   { "/Tools/FirewallACLRules",     NULL,                           "Firewall ACL Rules",       NULL,                       NULL,               G_CALLBACK(firewall_rule_cb) },

   { "/Internals/Dissectortables",  NULL,                           "_Dissector tables",        NULL,                       NULL,               G_CALLBACK(dissector_tables_dlg_cb) },
   { "/Internals/Conversations",    NULL,                           "_Conversation hash tables",NULL,                       NULL,               G_CALLBACK(conversation_hastables_dlg) },
   { "/Internals/SupportedProtocols", NULL,                 "_Supported Protocols (slow!)",     NULL,                       NULL,               G_CALLBACK(supported_cb) },

   { "/Help/Contents",              GTK_STOCK_HELP,                 "_Contents",            "F1",                           NULL,               G_CALLBACK(help_menu_cont_cb) },
   { "/Help/ManualPages",           NULL,                           "ManualPages",          NULL,                           NULL,               NULL },
   { "/Help/ManualPages/Wireshark", NULL,                           "Wireshark",            NULL,                           NULL,               G_CALLBACK(help_menu_wireshark_cb) },
   { "/Help/ManualPages/WiresharkFilter", NULL,                     "Wireshark Filter",     NULL,                           NULL,               G_CALLBACK(help_menu_wireshark_flt_cb) },
   { "/Help/ManualPages/Capinfos",  NULL,                           "Capinfos",              NULL,                          NULL,               G_CALLBACK(help_menu_Capinfos_cb) },
   { "/Help/ManualPages/Dumpcap",   NULL,                           "Dumpcap",              NULL,                           NULL,               G_CALLBACK(help_menu_Dumpcap_cb) },
   { "/Help/ManualPages/Editcap",   NULL,                           "Editcap",              NULL,                           NULL,               G_CALLBACK(help_menu_Editcap_cb) },
   { "/Help/ManualPages/Mergecap",  NULL,                           "Mergecap",             NULL,                           NULL,               G_CALLBACK(help_menu_Mergecap_cb) },
   { "/Help/ManualPages/RawShark",  NULL,                           "RawShark",             NULL,                           NULL,               G_CALLBACK(help_menu_RawShark_cb) },
   { "/Help/ManualPages/Reordercap",  NULL,                         "Reordercap",             NULL,                         NULL,               G_CALLBACK(help_menu_Reorder_cb) },
   { "/Help/ManualPages/Text2pcap", NULL,                           "Text2pcap",            NULL,                           NULL,               G_CALLBACK(help_menu_Text2pcap_cb) },
   { "/Help/ManualPages/TShark",    NULL,                           "TShark",            NULL,                           NULL,               G_CALLBACK(help_menu_TShark_cb) },

   { "/Help/Website",               GTK_STOCK_HOME,                 "Website",              NULL,                           NULL,               G_CALLBACK(help_menu_Website_cb) },
   { "/Help/FAQs",                  NULL,                           "FAQ's",                NULL,                           NULL,               G_CALLBACK(help_menu_faq_cb) },
   { "/Help/ASK",                   NULL,                           "Ask (Q&A)",            NULL,                           NULL,               G_CALLBACK(help_menu_ask_cb) },
   { "/Help/Downloads",             NULL,                           "Downloads",            NULL,                           NULL,               G_CALLBACK(help_menu_Downloads_cb) },
   { "/Help/Wiki",                  WIRESHARK_STOCK_WIKI,           "Wiki",                 NULL,                           NULL,               G_CALLBACK(help_menu_Wiki_cb) },
   { "/Help/SampleCaptures",        NULL,                           "Sample Captures",      NULL,                           NULL,               G_CALLBACK(help_menu_SampleCaptures_cb) },
#ifdef HAVE_SOFTWARE_UPDATE
   { "/Help/CheckForUpdates",       NULL,                           "Check for Updates...", NULL,                           NULL,               G_CALLBACK(check_for_updates_cb) },
#endif /* HAVE_SOFTWARE_UPDATE */
   { "/Help/AboutWireshark",        WIRESHARK_STOCK_ABOUT,          "_About Wireshark",     NULL,                           NULL,               G_CALLBACK(about_wireshark_cb) },
};

static const GtkToggleActionEntry main_menu_bar_toggle_action_entries[] =
{
    /* name, stock id, label, accel, tooltip, callback, is_active */
    {"/View/MainToolbar",                                           NULL, "_Main Toolbar",                          NULL, NULL, G_CALLBACK(main_toolbar_show_hide_cb), TRUE},
    {"/View/FilterToolbar",                                         NULL, "_Filter Toolbar",                         NULL, NULL, G_CALLBACK(filter_toolbar_show_hide_cb), TRUE},
    {"/View/WirelessToolbar",                                       NULL, "Wire_less Toolbar",                       NULL, NULL, G_CALLBACK(wireless_toolbar_show_hide_cb), FALSE},
    {"/View/StatusBar",                                             NULL, "_Status Bar",                             NULL, NULL, G_CALLBACK(status_bar_show_hide_cb), TRUE},
    {"/View/PacketList",                                            NULL, "Packet _List",                           NULL, NULL, G_CALLBACK(packet_list_show_hide_cb), TRUE},
    {"/View/PacketDetails",                                         NULL, "Packet _Details",                        NULL, NULL, G_CALLBACK(packet_details_show_hide_cb), TRUE},
    {"/View/PacketBytes",                                           NULL, "Packet _Bytes",                          NULL, NULL, G_CALLBACK(packet_bytes_show_hide_cb), TRUE},
    {"/View/TimeDisplayFormat/DisplaySecondsWithHoursAndMinutes",   NULL, "Display Seconds with hours and minutes", NULL, NULL, G_CALLBACK(timestamp_seconds_time_cb), FALSE},
    {"/View/NameResolution/EnableforMACLayer",                      NULL, "Enable for _MAC Layer",                  NULL, NULL, G_CALLBACK(view_menu_en_for_MAC_cb), TRUE},
    {"/View/NameResolution/EnableforTransportLayer",                NULL, "Enable for _Transport Layer",            NULL, NULL, G_CALLBACK(view_menu_en_for_transport_cb), TRUE },
    {"/View/NameResolution/EnableforNetworkLayer",                  NULL, "Enable for _Network Layer",              NULL, NULL, G_CALLBACK(view_menu_en_for_network_cb), TRUE },
    {"/View/NameResolution/UseExternalNetworkNameResolver",         NULL, "Use _External Network Name Resolver",    NULL, NULL, G_CALLBACK(view_menu_en_use_external_resolver_cb), TRUE },
    {"/View/ColorizePacketList",                                    NULL, "Colorize Packet List",                   NULL, NULL, G_CALLBACK(view_menu_colorize_pkt_lst_cb), TRUE },
#ifdef HAVE_LIBPCAP
    {"/View/AutoScrollinLiveCapture",                               NULL, "Auto Scroll in Li_ve Capture",           NULL, NULL, G_CALLBACK(view_menu_auto_scroll_live_cb), TRUE },
#endif
};

static const GtkRadioActionEntry main_menu_bar_radio_view_time_entries [] =
{
    /* name, stock id, label, accel, tooltip,  value */
    { "/View/TimeDisplayFormat/DateYMDandTimeofDay",                NULL, "Date and Time of Day:   1970-01-01 01:02:03.123456", "<alt><control>1", NULL, TS_ABSOLUTE_WITH_YMD },
    { "/View/TimeDisplayFormat/DateYDOYandTimeofDay",               NULL, "Date (with day of year) and Time of Day:   1970/001 01:02:03.123456", NULL, NULL, TS_ABSOLUTE_WITH_YDOY },
    { "/View/TimeDisplayFormat/TimeofDay",                          NULL, "Time of Day:   01:02:03.123456", "<alt><control>2", NULL, TS_ABSOLUTE },
    { "/View/TimeDisplayFormat/SecondsSinceEpoch",                  NULL, "Seconds Since Epoch (1970-01-01):   1234567890.123456", "<alt><control>3", NULL, TS_EPOCH },
    { "/View/TimeDisplayFormat/SecondsSinceBeginningofCapture",     NULL, "Seconds Since Beginning of Capture:   123.123456", "<alt><control>4", NULL, TS_RELATIVE },
    { "/View/TimeDisplayFormat/SecondsSincePreviousCapturedPacket", NULL, "Seconds Since Previous Captured Packet:   1.123456", "<alt><control>5", NULL, TS_DELTA },
    { "/View/TimeDisplayFormat/SecondsSincePreviousDisplayedPacket",NULL, "Seconds Since Previous Displayed Packet:   1.123456", "<alt><control>6", NULL, TS_DELTA_DIS },
    { "/View/TimeDisplayFormat/UTCDateYMDandTimeofDay",             NULL, "UTC Date and Time of Day:   1970-01-01 01:02:03.123456", "<alt><control>7", NULL, TS_UTC_WITH_YMD },
    { "/View/TimeDisplayFormat/UTCDateYDOYandTimeofDay",            NULL, "UTC Date (with day of year) and Time of Day:   1970/001 01:02:03.123456", NULL, NULL, TS_UTC_WITH_YDOY },
    { "/View/TimeDisplayFormat/UTCTimeofDay",                       NULL, "UTC Time of Day:   01:02:03.123456", "<alt><control>8", NULL, TS_UTC },
};

static const GtkRadioActionEntry main_menu_bar_radio_view_time_fileformat_prec_entries [] =
{
    /* name, stock id, label, accel, tooltip,  value */
    { "/View/TimeDisplayFormat/FileFormatPrecision-Automatic",      NULL, "Automatic (use precision indicated in the file)",  NULL, NULL, TS_PREC_AUTO },
    { "/View/TimeDisplayFormat/FileFormatPrecision-Seconds",        NULL, "Seconds:   0",                       NULL, NULL, TS_PREC_FIXED_SEC },
    { "/View/TimeDisplayFormat/FileFormatPrecision-Deciseconds",    NULL, "Deciseconds:   0.1",                 NULL, NULL, TS_PREC_FIXED_DSEC },
    { "/View/TimeDisplayFormat/FileFormatPrecision-Centiseconds",   NULL, "Centiseconds:  0.12",                NULL, NULL, TS_PREC_FIXED_CSEC },
    { "/View/TimeDisplayFormat/FileFormatPrecision-Milliseconds",   NULL, "Milliseconds:  0.123",               NULL, NULL, TS_PREC_FIXED_MSEC },
    { "/View/TimeDisplayFormat/FileFormatPrecision-Microseconds",   NULL, "Microseconds:  0.123456",            NULL, NULL, TS_PREC_FIXED_USEC },
    { "/View/TimeDisplayFormat/FileFormatPrecision-Nanoseconds",    NULL, "Nanoseconds:   0.123456789",         NULL, NULL, TS_PREC_FIXED_NSEC },
};


static void
select_bytes_view_cb (GtkRadioAction *action, GtkRadioAction *current _U_, gpointer user_data _U_)
{
    bytes_view_type value;

    value = (bytes_view_type)gtk_radio_action_get_current_value (action);
    /* Fix me */
    select_bytes_view( NULL, NULL, value);
}

static void
sort_ascending_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_packet_list_heading, "/PacketListHeadingPopup/SortAscending");
    packet_list_column_menu_cb( widget , user_data, COLUMN_SELECTED_SORT_ASCENDING);
}

static void
sort_descending_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_packet_list_heading, "/PacketListHeadingPopup/SortDescending");
    packet_list_column_menu_cb( widget , user_data, COLUMN_SELECTED_SORT_DESCENDING);
}

static void
no_sorting_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_packet_list_heading, "/PacketListHeadingPopup/NoSorting");
    packet_list_column_menu_cb( widget , user_data, COLUMN_SELECTED_SORT_NONE);
}

static void
packet_list_heading_show_resolved_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_packet_list_heading, "/PacketListHeadingPopup/ShowResolved");

    packet_list_column_menu_cb( widget , user_data, COLUMN_SELECTED_TOGGLE_RESOLVED);
}

static void
packet_list_heading_align_left_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_packet_list_heading, "/PacketListHeadingPopup/AlignLeft");
    packet_list_column_menu_cb( widget , user_data, COLUMN_SELECTED_ALIGN_LEFT);
}

static void
packet_list_heading_align_center_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_packet_list_heading, "/PacketListHeadingPopup/AlignCenter");
    packet_list_column_menu_cb( widget , user_data, COLUMN_SELECTED_ALIGN_CENTER);
}

static void
packet_list_heading_align_right_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_packet_list_heading, "/PacketListHeadingPopup/AlignRight");
    packet_list_column_menu_cb( widget , user_data, COLUMN_SELECTED_ALIGN_RIGHT);
}

static void
packet_list_heading_col_pref_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_packet_list_heading, "/PacketListHeadingPopup/ColumnPreferences");
    prefs_page_cb( widget , user_data, PREFS_PAGE_COLUMNS);
}

static void
packet_list_heading_resize_col_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_packet_list_heading, "/PacketListHeadingPopup/ResizeColumn");
    packet_list_column_menu_cb( widget , user_data, COLUMN_SELECTED_RESIZE);
}

static void
packet_list_heading_change_col_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_packet_list_heading, "/PacketListHeadingPopup/EditColumnDetails");
    packet_list_column_menu_cb( widget , user_data, COLUMN_SELECTED_CHANGE);
}

static void
packet_list_heading_activate_all_columns_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    packet_list_set_all_columns_visible ();
}

static void
packet_list_heading_hide_col_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_packet_list_heading, "/PacketListHeadingPopup/HideColumn");
    packet_list_column_menu_cb( widget , user_data, COLUMN_SELECTED_HIDE);
}

static void
packet_list_heading_remove_col_cb(GtkAction *action _U_, gpointer user_data)
{
    GtkWidget *widget = gtk_ui_manager_get_widget(ui_manager_packet_list_heading, "/PacketListHeadingPopup/RemoveColumn");
    packet_list_column_menu_cb( widget , user_data, COLUMN_SELECTED_REMOVE);
}

static void
packet_list_menu_set_ref_time_cb(GtkAction *action _U_, gpointer user_data)
{
    reftime_frame_cb( NULL /* widget _U_ */ , user_data, REFTIME_TOGGLE);
}


static void
apply_selected_filter_cb(GtkAction *action, gpointer user_data)
{
    const gchar *path = gtk_action_get_accel_path(action);
    /*g_warning("Accelerator path %s",path+9);*/

    /* path starts with "<Actions>" */
    if (strncmp (path+9,"/PacketListPopUpMenuActionGroup",31) == 0){
        /* Use different callbacks depending action path */
        match_selected_plist_cb(user_data, (MATCH_SELECTED_E)(MATCH_SELECTED_REPLACE|MATCH_SELECTED_APPLY_NOW));
    } else {
        match_selected_ptree_cb(user_data, (MATCH_SELECTED_E)(MATCH_SELECTED_REPLACE|MATCH_SELECTED_APPLY_NOW));
    }
}

static void
apply_not_selected_cb(GtkAction *action, gpointer user_data)
{
    const gchar *path = gtk_action_get_accel_path(action);

    /* path starts with "<Actions>" */
    if (strncmp (path+9,"/PacketListPopUpMenuActionGroup",31) == 0){
        /* Use different callbacks depending action path */
        match_selected_plist_cb(user_data, (MATCH_SELECTED_E)(MATCH_SELECTED_NOT|MATCH_SELECTED_APPLY_NOW));
    } else {
        match_selected_ptree_cb(user_data, (MATCH_SELECTED_E)(MATCH_SELECTED_NOT|MATCH_SELECTED_APPLY_NOW));
    }
}

static void
apply_and_selected_cb(GtkAction *action, gpointer user_data)
{
    const gchar *path = gtk_action_get_accel_path(action);

    /* path starts with "<Actions>" */
    if (strncmp (path+9,"/PacketListPopUpMenuActionGroup",31) == 0){
        /* Use different callbacks depending action path */
        match_selected_plist_cb(user_data, (MATCH_SELECTED_E)(MATCH_SELECTED_AND|MATCH_SELECTED_APPLY_NOW));
    } else {
        match_selected_ptree_cb(user_data, (MATCH_SELECTED_E)(MATCH_SELECTED_AND|MATCH_SELECTED_APPLY_NOW));
    }
}

static void
apply_or_selected_cb(GtkAction *action, gpointer user_data)
{
    const gchar *path = gtk_action_get_accel_path(action);

    /* path starts with "<Actions>" */
    if (strncmp (path+9,"/PacketListPopUpMenuActionGroup",31) == 0){
        /* Use different callbacks depending action path */
        match_selected_plist_cb(user_data, (MATCH_SELECTED_E)(MATCH_SELECTED_OR|MATCH_SELECTED_APPLY_NOW));
    } else {
        match_selected_ptree_cb(user_data, (MATCH_SELECTED_E)(MATCH_SELECTED_OR|MATCH_SELECTED_APPLY_NOW));
    }
}

static void
apply_and_not_selected_cb(GtkAction *action, gpointer user_data)
{
    const gchar *path = gtk_action_get_accel_path(action);

    /* path starts with "<Actions>" */
    if (strncmp (path+9,"/PacketListPopUpMenuActionGroup",31) == 0){
        /* Use different callbacks depending action path */
        match_selected_plist_cb(user_data, (MATCH_SELECTED_E)(MATCH_SELECTED_AND_NOT|MATCH_SELECTED_APPLY_NOW));
    } else {
        match_selected_ptree_cb(user_data, (MATCH_SELECTED_E)(MATCH_SELECTED_AND_NOT|MATCH_SELECTED_APPLY_NOW));
    }
}

static void
apply_or_not_selected_cb(GtkAction *action, gpointer user_data)
{
    const gchar *path = gtk_action_get_accel_path(action);

    /* path starts with "<Actions>" */
    if (strncmp (path+9,"/PacketListPopUpMenuActionGroup",31) == 0){
        /* Use different callbacks depending action path */
        match_selected_plist_cb(user_data,(MATCH_SELECTED_E)(MATCH_SELECTED_OR_NOT|MATCH_SELECTED_APPLY_NOW));
    } else {
        match_selected_ptree_cb(user_data, (MATCH_SELECTED_E)(MATCH_SELECTED_OR_NOT|MATCH_SELECTED_APPLY_NOW));
    }
}

/* Prepare a filter */
static void
prepare_selected_cb(GtkAction *action, gpointer user_data)
{
    const gchar *path = gtk_action_get_accel_path(action);

    /* path starts with "<Actions>" */
    if (strncmp (path+9,"/PacketListPopUpMenuActionGroup",31) == 0){
        /* Use different callbacks depending action path */
        match_selected_plist_cb(user_data, MATCH_SELECTED_REPLACE);
    } else {
        match_selected_ptree_cb(user_data, MATCH_SELECTED_REPLACE);
    }
}

static void
prepare_not_selected_cb(GtkAction *action, gpointer user_data)
{
    const gchar *path = gtk_action_get_accel_path(action);

    /* path starts with "<Actions>" */
    if (strncmp (path+9,"/PacketListPopUpMenuActionGroup",31) == 0){
        /* Use different callbacks depending action path */
        match_selected_plist_cb(user_data, MATCH_SELECTED_NOT);
    } else {
        match_selected_ptree_cb(user_data, MATCH_SELECTED_NOT);
    }
}

static void
prepare_and_selected_cb(GtkAction *action, gpointer user_data)
{
    const gchar *path = gtk_action_get_accel_path(action);

    /* path starts with "<Actions>" */
    if (strncmp (path+9,"/PacketListPopUpMenuActionGroup",31) == 0){
        /* Use different callbacks depending action path */
        match_selected_plist_cb(user_data, MATCH_SELECTED_AND);
    } else {
        match_selected_ptree_cb(user_data, MATCH_SELECTED_AND);
    }
}

static void
prepare_or_selected_cb(GtkAction *action, gpointer user_data)
{
    const gchar *path = gtk_action_get_accel_path(action);

    /* path starts with "<Actions>" */
    if (strncmp (path+9,"/PacketListPopUpMenuActionGroup",31) == 0){
        /* Use different callbacks depending action path */
        match_selected_plist_cb(user_data, MATCH_SELECTED_OR);
    } else {
        match_selected_ptree_cb(user_data, MATCH_SELECTED_OR);
    }
}

static void
prepare_and_not_selected_cb(GtkAction *action, gpointer user_data)
{
    const gchar *path = gtk_action_get_accel_path(action);

    /* path starts with "<Actions>" */
    if (strncmp (path+9,"/PacketListPopUpMenuActionGroup",31) == 0){
        /* Use different callbacks depending action path */
        match_selected_plist_cb(user_data, MATCH_SELECTED_AND_NOT);
    } else {
        match_selected_ptree_cb(user_data, MATCH_SELECTED_AND_NOT);
    }
}

static void
prepare_or_not_selected_cb(GtkAction *action, gpointer user_data)
{
    const gchar *path = gtk_action_get_accel_path(action);

    /* path starts with "<Actions>" */
    if (strncmp (path+9,"/PacketListPopUpMenuActionGroup",31) == 0){
        /* Use different callbacks depending action path */
        match_selected_plist_cb(user_data, MATCH_SELECTED_OR_NOT);
    } else {
        match_selected_ptree_cb(user_data, MATCH_SELECTED_OR_NOT);
    }
}

typedef void (*packet_list_menu_color_conv_color_cb_t)(GtkAction *action, gpointer user_data);

static void
packet_list_menu_color_conv_color1_cb(GtkAction *action _U_, gpointer user_data)
{
    conversation_filter_t* color_filter = (conversation_filter_t*)user_data;
    colorize_conversation_cb(color_filter, 1);
}

static void
packet_list_menu_color_conv_color2_cb(GtkAction *action _U_, gpointer user_data)
{
    conversation_filter_t* color_filter = (conversation_filter_t*)user_data;
    colorize_conversation_cb(color_filter, 2);
}

static void
packet_list_menu_color_conv_color3_cb(GtkAction *action _U_, gpointer user_data)
{
    conversation_filter_t* color_filter = (conversation_filter_t*)user_data;
    colorize_conversation_cb(color_filter, 3);
}

static void
packet_list_menu_color_conv_color4_cb(GtkAction *action _U_, gpointer user_data)
{
    conversation_filter_t* color_filter = (conversation_filter_t*)user_data;
    colorize_conversation_cb(color_filter, 4);
}

static void
packet_list_menu_color_conv_color5_cb(GtkAction *action _U_, gpointer user_data)
{
    conversation_filter_t* color_filter = (conversation_filter_t*)user_data;
    colorize_conversation_cb(color_filter, 5);
}

static void
packet_list_menu_color_conv_color6_cb(GtkAction *action _U_, gpointer user_data)
{
    conversation_filter_t* color_filter = (conversation_filter_t*)user_data;
    colorize_conversation_cb(color_filter, 6);
}

static void
packet_list_menu_color_conv_color7_cb(GtkAction *action _U_, gpointer user_data)
{
    conversation_filter_t* color_filter = (conversation_filter_t*)user_data;
    colorize_conversation_cb(color_filter, 7);
}

static void
packet_list_menu_color_conv_color8_cb(GtkAction *action _U_, gpointer user_data)
{
    conversation_filter_t* color_filter = (conversation_filter_t*)user_data;
    colorize_conversation_cb(color_filter, 8);
}

static void
packet_list_menu_color_conv_color9_cb(GtkAction *action _U_, gpointer user_data)
{
    conversation_filter_t* color_filter = (conversation_filter_t*)user_data;
    colorize_conversation_cb(color_filter, 9);
}

static void
packet_list_menu_color_conv_color10_cb(GtkAction *action _U_, gpointer user_data)
{
    conversation_filter_t* color_filter = (conversation_filter_t*)user_data;
    colorize_conversation_cb(color_filter, 10);
}

static void
packet_list_menu_color_conv_new_rule_cb(GtkAction *action _U_, gpointer user_data)
{
    conversation_filter_t* color_filter = (conversation_filter_t*)user_data;
    colorize_conversation_cb(color_filter, 0);
}

static void
packet_list_menu_copy_sum_txt(GtkAction *action _U_, gpointer user_data)
{
    packet_list_copy_summary_cb(user_data, CS_TEXT);
}

static void
packet_list_menu_copy_sum_csv(GtkAction *action _U_, gpointer user_data)
{
    packet_list_copy_summary_cb(user_data, CS_CSV);
}

static void
packet_list_menu_copy_as_flt(GtkAction *action _U_, gpointer user_data)
{
    match_selected_plist_cb(user_data, (MATCH_SELECTED_E)(MATCH_SELECTED_REPLACE|MATCH_SELECTED_COPY_ONLY));
}

static void
packet_list_menu_copy_bytes_oht_cb(GtkAction *action _U_, gpointer user_data)
{
    copy_hex_cb( NULL /* widget _U_ */ , user_data, (copy_data_type)(CD_ALLINFO | CD_FLAGS_SELECTEDONLY));
}

static void
packet_list_menu_copy_bytes_oh_cb(GtkAction *action _U_, gpointer user_data)
{
    copy_hex_cb( NULL /* widget _U_ */ , user_data, (copy_data_type)(CD_HEXCOLUMNS | CD_FLAGS_SELECTEDONLY));
}

static void
packet_list_menu_copy_bytes_text_cb(GtkAction *action _U_, gpointer user_data)
{
    copy_hex_cb( NULL /* widget _U_ */ , user_data, (copy_data_type)(CD_TEXTONLY | CD_FLAGS_SELECTEDONLY));
}

static void
packet_list_menu_copy_bytes_hex_strm_cb(GtkAction *action _U_, gpointer user_data)
{
    copy_hex_cb( NULL /* widget _U_ */ , user_data,  (copy_data_type)(CD_HEX | CD_FLAGS_SELECTEDONLY));
}

static void
packet_list_menu_copy_bytes_bin_strm_cb(GtkAction *action _U_, gpointer user_data)
{
    copy_hex_cb( NULL /* widget _U_ */ , user_data, (copy_data_type)(CD_BINARY | CD_FLAGS_SELECTEDONLY));
}

/* tree */

static void
tree_view_menu_color_with_flt_color1_cb(GtkAction *action _U_, gpointer user_data)
{
    colorize_selected_ptree_cb( NULL /* widget _U_ */ , user_data, 1);
}

static void
tree_view_menu_color_with_flt_color2_cb(GtkAction *action _U_, gpointer user_data)
{
    colorize_selected_ptree_cb( NULL /* widget _U_ */ , user_data, 2);
}

static void
tree_view_menu_color_with_flt_color3_cb(GtkAction *action _U_, gpointer user_data)
{
    colorize_selected_ptree_cb( NULL /* widget _U_ */ , user_data, 3);
}

static void
tree_view_menu_color_with_flt_color4_cb(GtkAction *action _U_, gpointer user_data)
{
    colorize_selected_ptree_cb( NULL /* widget _U_ */ , user_data, 4);
}

static void
tree_view_menu_color_with_flt_color5_cb(GtkAction *action _U_, gpointer user_data)
{
    colorize_selected_ptree_cb( NULL /* widget _U_ */ , user_data, 5);
}

static void
tree_view_menu_color_with_flt_color6_cb(GtkAction *action _U_, gpointer user_data)
{
    colorize_selected_ptree_cb( NULL /* widget _U_ */ , user_data, 6);
}

static void
tree_view_menu_color_with_flt_color7_cb(GtkAction *action _U_, gpointer user_data)
{
    colorize_selected_ptree_cb( NULL /* widget _U_ */ , user_data, 7);
}

static void
tree_view_menu_color_with_flt_color8_cb(GtkAction *action _U_, gpointer user_data)
{
    colorize_selected_ptree_cb( NULL /* widget _U_ */ , user_data, 8);
}

static void
tree_view_menu_color_with_flt_color9_cb(GtkAction *action _U_, gpointer user_data)
{
    colorize_selected_ptree_cb( NULL /* widget _U_ */ , user_data, 9);
}

static void
tree_view_menu_color_with_flt_color10_cb(GtkAction *action _U_, gpointer user_data)
{
    colorize_selected_ptree_cb( NULL /* widget _U_ */ , user_data, 10);
}

static void
tree_view_menu_color_with_flt_new_rule_cb(GtkAction *action _U_, gpointer user_data)
{
    colorize_selected_ptree_cb( NULL /* widget _U_ */ , user_data, 0);
}


static void
tree_view_menu_copy_desc(GtkAction *action _U_, gpointer user_data)
{
    copy_selected_plist_cb( NULL /* widget _U_ */ , user_data, COPY_SELECTED_DESCRIPTION);
}

static void
tree_view_menu_copy_field(GtkAction *action _U_, gpointer user_data)
{
    copy_selected_plist_cb( NULL /* widget _U_ */ , user_data, COPY_SELECTED_FIELDNAME);
}

static void
tree_view_menu_copy_value(GtkAction *action _U_, gpointer user_data)
{
    copy_selected_plist_cb( NULL /* widget _U_ */ , user_data, COPY_SELECTED_VALUE);
}

static void
tree_view_menu_copy_as_flt(GtkAction *action _U_, gpointer user_data _U_)
{
    /* match_selected_ptree_cb needs the popup_menu_object to get the right object E_DFILTER_TE_KEY */
    match_selected_ptree_cb( popup_menu_object, (MATCH_SELECTED_E)(MATCH_SELECTED_REPLACE|MATCH_SELECTED_COPY_ONLY));
}

static const char *ui_desc_packet_list_heading_menu_popup =
"<ui>\n"
"  <popup name='PacketListHeadingPopup' action='PopupAction'>\n"
"     <menuitem name='SortAscending' action='/Sort Ascending'/>\n"
"     <menuitem name='SortDescending' action='/Sort Descending'/>\n"
"     <menuitem name='NoSorting' action='/No Sorting'/>\n"
"     <separator/>\n"
"     <menuitem name='ShowResolved' action='/Show Resolved'/>\n"
"     <separator/>\n"
"     <menuitem name='AlignLeft' action='/Align Left'/>\n"
"     <menuitem name='AlignCenter' action='/Align Center'/>\n"
"     <menuitem name='AlignRight' action='/Align Right'/>\n"
"     <separator/>\n"
"     <menuitem name='ColumnPreferences' action='/Column Preferences'/>\n"
"     <menuitem name='EditColumnDetails' action='/Edit Column Details'/>\n"
"     <menuitem name='ResizeColumn' action='/Resize Column'/>\n"
"     <separator/>\n"
"     <menu name='DisplayedColumns' action='/Displayed Columns'>\n"
"       <menuitem name='Display All' action='/Displayed Columns/Display All'/>\n"
"     </menu>\n"
"     <menuitem name='HideColumn' action='/Hide Column'/>\n"
"     <menuitem name='RemoveColumn' action='/Remove Column'/>\n"
"  </popup>\n"
"</ui>\n";

static const GtkActionEntry packet_list_heading_menu_popup_action_entries[] = {
  { "/Sort Ascending",                  GTK_STOCK_SORT_ASCENDING,           "Sort Ascending",           NULL,   NULL,   G_CALLBACK(sort_ascending_cb) },
  { "/Sort Descending",                 GTK_STOCK_SORT_DESCENDING,          "Sort Descending",          NULL,   NULL,   G_CALLBACK(sort_descending_cb) },
  { "/No Sorting",                      NULL,                               "No Sorting",               NULL,   NULL,   G_CALLBACK(no_sorting_cb) },
  { "/Align Left",                      GTK_STOCK_JUSTIFY_LEFT,             "Align Left",               NULL,   NULL,   G_CALLBACK(packet_list_heading_align_left_cb) },
  { "/Align Center",                    GTK_STOCK_JUSTIFY_CENTER,           "Align Center",             NULL,   NULL,   G_CALLBACK(packet_list_heading_align_center_cb) },
  { "/Align Right",                     GTK_STOCK_JUSTIFY_RIGHT,            "Align Right",              NULL,   NULL,   G_CALLBACK(packet_list_heading_align_right_cb) },
  { "/Column Preferences",              GTK_STOCK_PREFERENCES,              "Column Preferences...",    NULL,   NULL,   G_CALLBACK(packet_list_heading_col_pref_cb) },
  { "/Edit Column Details",             WIRESHARK_STOCK_EDIT,           "Edit Column Details...",       NULL,   NULL,   G_CALLBACK(packet_list_heading_change_col_cb) },
  { "/Resize Column",                   WIRESHARK_STOCK_RESIZE_COLUMNS,     "Resize Column",            NULL,   NULL,   G_CALLBACK(packet_list_heading_resize_col_cb) },
  { "/Displayed Columns",               NULL,                               "Displayed Columns",        NULL,   NULL,   NULL },
  { "/Displayed Columns/Display All",               NULL,                   "Display All",              NULL,   NULL,   G_CALLBACK(packet_list_heading_activate_all_columns_cb) },
  { "/Hide Column",                     NULL,                               "Hide Column",              NULL,   NULL,   G_CALLBACK(packet_list_heading_hide_col_cb) },
  { "/Remove Column",                   GTK_STOCK_DELETE,                   "Remove Column",            NULL,   NULL,   G_CALLBACK(packet_list_heading_remove_col_cb) },
};

static const GtkToggleActionEntry packet_list_heading_menu_toggle_action_entries[] =
{
    /* name, stock id, label, accel, tooltip, callback, is_active */
    {"/Show Resolved",  NULL, "Show Resolved",  NULL, NULL, G_CALLBACK(packet_list_heading_show_resolved_cb), FALSE},
};

static const char *ui_desc_packet_list_menu_popup =
"<ui>\n"
"  <popup name='PacketListMenuPopup' action='PopupAction'>\n"
"     <menuitem name='MarkPacket' action='/MarkPacket'/>\n"
"     <menuitem name='IgnorePacket' action='/IgnorePacket'/>\n"
"     <menuitem name='SetTimeReference' action='/Set Time Reference'/>\n"
"     <menuitem name='TimeShift' action='/TimeShift'/>\n"
#ifdef WANT_PACKET_EDITOR
"     <menuitem name='EditPacket' action='/Edit/EditPacket'/>\n"
#endif
"     <menuitem name='AddEditPktComment' action='/Edit/AddEditPktComment'/>\n"
"     <separator/>\n"
"     <menuitem name='ManuallyResolveAddress' action='/ManuallyResolveAddress'/>\n"
"     <separator/>\n"
"     <menu name= 'ApplyAsFilter' action='/ApplyasFilter'>\n"
"       <menuitem name='Selected' action='/ApplyasFilter/Selected'/>\n"
"       <menuitem name='NotSelected' action='/ApplyasFilter/Not Selected'/>\n"
"       <menuitem name='AndSelected' action='/ApplyasFilter/AndSelected'/>\n"
"       <menuitem name='OrSelected' action='/ApplyasFilter/OrSelected'/>\n"
"       <menuitem name='AndNotSelected' action='/ApplyasFilter/AndNotSelected'/>\n"
"       <menuitem name='OrNotSelected' action='/ApplyasFilter/OrNotSelected'/>\n"
"     </menu>\n"
"     <menu name= 'PrepareaFilter' action='/PrepareaFilter'>\n"
"       <menuitem name='Selected' action='/PrepareaFilter/Selected'/>\n"
"       <menuitem name='NotSelected' action='/PrepareaFilter/Not Selected'/>\n"
"       <menuitem name='AndSelected' action='/PrepareaFilter/AndSelected'/>\n"
"       <menuitem name='OrSelected' action='/PrepareaFilter/OrSelected'/>\n"
"       <menuitem name='AndNotSelected' action='/PrepareaFilter/AndNotSelected'/>\n"
"       <menuitem name='OrNotSelected' action='/PrepareaFilter/OrNotSelected'/>\n"
"     </menu>\n"
"     <menu name= 'ConversationFilter' action='/Conversation Filter'>\n"
"       <placeholder name='Conversations'/>\n"
"     </menu>\n"
"     <menu name= 'ColorizeConversation' action='/Colorize Conversation'>\n"
"       <placeholder name='Colorize'/>\n"
"     </menu>\n"
"     <menu name= 'SCTP' action='/SCTP'>\n"
"        <menuitem name='AnalysethisAssociation' action='/SCTP/Analyse this Association'/>\n"
"        <menuitem name='PrepareFilterforthisAssociation' action='/SCTP/Prepare Filter for this Association'/>\n"
"     </menu>\n"
"     <menuitem name='FollowTCPStream' action='/Follow TCP Stream'/>\n"
"     <menuitem name='FollowUDPStream' action='/Follow UDP Stream'/>\n"
"     <menuitem name='FollowSSLStream' action='/Follow SSL Stream'/>\n"
"     <menuitem name='FollowHTTPStream' action='/Follow HTTP Stream'/>\n"
"     <separator/>\n"
"     <menu name= 'Copy' action='/Copy'>\n"
"        <menuitem name='SummaryTxt' action='/Copy/SummaryTxt'/>\n"
"        <menuitem name='SummaryCSV' action='/Copy/SummaryCSV'/>\n"
"        <menuitem name='AsFilter' action='/Copy/AsFilter'/>\n"
"        <separator/>\n"
"        <menu name= 'Bytes' action='/Copy/Bytes'>\n"
"           <menuitem name='OffsetHexText' action='/Copy/Bytes/OffsetHexText'/>\n"
"           <menuitem name='OffsetHex' action='/Copy/Bytes/OffsetHex'/>\n"
"           <menuitem name='PrintableTextOnly' action='/Copy/Bytes/PrintableTextOnly'/>\n"
"           <separator/>\n"
"           <menuitem name='HexStream' action='/Copy/Bytes/HexStream'/>\n"
"           <menuitem name='BinaryStream' action='/Copy/Bytes/BinaryStream'/>\n"
"        </menu>\n"
"     </menu>\n"
"     <separator/>\n"
"     <menuitem name='ProtocolPreferences' action='/ProtocolPreferences'/>\n"
"     <menuitem name='DecodeAs' action='/DecodeAs'/>\n"
"     <menuitem name='Print' action='/Print'/>\n"
"     <menuitem name='ShowPacketinNewWindow' action='/ShowPacketinNewWindow'/>\n"
"  </popup>\n"
"</ui>\n";

static const GtkActionEntry apply_prepare_filter_action_entries[] = {
  { "/ApplyasFilter",                 NULL, "Apply as Filter",                             NULL, NULL, NULL },
  { "/ApplyasFilter/Selected",        NULL, "_Selected" ,                                  NULL, NULL, G_CALLBACK(apply_selected_filter_cb) },
  { "/ApplyasFilter/Not Selected",    NULL, "_Not Selected",                               NULL, NULL, G_CALLBACK(apply_not_selected_cb) },
  { "/ApplyasFilter/AndSelected",     NULL, UTF8_HORIZONTAL_ELLIPSIS " _and Selected",     NULL, NULL, G_CALLBACK(apply_and_selected_cb) },
  { "/ApplyasFilter/OrSelected",      NULL, UTF8_HORIZONTAL_ELLIPSIS " _or Selected",      NULL, NULL, G_CALLBACK(apply_or_selected_cb) },
  { "/ApplyasFilter/AndNotSelected",  NULL, UTF8_HORIZONTAL_ELLIPSIS " a_nd not Selected", NULL, NULL, G_CALLBACK(apply_and_not_selected_cb) },
  { "/ApplyasFilter/OrNotSelected",   NULL, UTF8_HORIZONTAL_ELLIPSIS " o_r not Selected",  NULL, NULL, G_CALLBACK(apply_or_not_selected_cb) },

  { "/PrepareaFilter",                NULL, "Prepare a Filter",       NULL, NULL, NULL },
  { "/PrepareaFilter/Selected",       NULL, "_Selected" ,             NULL, NULL, G_CALLBACK(prepare_selected_cb) },
  { "/PrepareaFilter/Not Selected",   NULL, "_Not Selected",          NULL, NULL, G_CALLBACK(prepare_not_selected_cb) },
  { "/PrepareaFilter/AndSelected",    NULL, UTF8_HORIZONTAL_ELLIPSIS " _and Selected",        NULL, NULL, G_CALLBACK(prepare_and_selected_cb) },
  { "/PrepareaFilter/OrSelected",     NULL, UTF8_HORIZONTAL_ELLIPSIS " _or Selected",     NULL, NULL, G_CALLBACK(prepare_or_selected_cb) },
  { "/PrepareaFilter/AndNotSelected", NULL, UTF8_HORIZONTAL_ELLIPSIS " a_nd not Selected",    NULL, NULL, G_CALLBACK(prepare_and_not_selected_cb) },
  { "/PrepareaFilter/OrNotSelected",  NULL, UTF8_HORIZONTAL_ELLIPSIS " o_r not Selected", NULL, NULL, G_CALLBACK(prepare_or_not_selected_cb) },
};


static const GtkActionEntry packet_list_menu_popup_action_entries[] = {
  { "/MarkPacket",                      NULL,                   "Mark Packet (toggle)",         NULL,                   NULL,           G_CALLBACK(packet_list_mark_frame_cb) },
  { "/IgnorePacket",                    NULL,                   "Ignore Packet (toggle)",       NULL,                   NULL,           G_CALLBACK(packet_list_ignore_frame_cb) },
  { "/Set Time Reference",              WIRESHARK_STOCK_TIME,   "Set Time Reference (toggle)",  NULL,                   NULL,           G_CALLBACK(packet_list_menu_set_ref_time_cb) },
  { "/TimeShift",                       WIRESHARK_STOCK_TIME,   "Time Shift...",                NULL,                   NULL,           G_CALLBACK(time_shift_cb) },
  { "/ManuallyResolveAddress",          NULL,                   "Manually Resolve Address",     NULL,                   NULL,           G_CALLBACK(manual_addr_resolv_dlg) },
#ifdef WANT_PACKET_EDITOR
   { "/Edit/EditPacket",                NULL,                   "_Edit Packet",                 NULL,                   NULL,           G_CALLBACK(edit_window_cb) },
#endif
  { "/Edit/AddEditPktComment",          WIRESHARK_STOCK_EDIT,   "Packet Comment...",            NULL,                   NULL,           G_CALLBACK(edit_packet_comment_dlg) },

  { "/Conversation Filter",             NULL, "Conversation Filter",    NULL, NULL, NULL },
  { "/Colorize Conversation",           NULL, "Colorize Conversation",  NULL, NULL, NULL },

  { "/SCTP",        NULL, "SCTP",               NULL, NULL, NULL },
  { "/SCTP/Analyse this Association",               NULL,       "Analyse this Association",             NULL, NULL, G_CALLBACK(sctp_analyse_start) },
  { "/SCTP/Prepare Filter for this Association",    NULL,       "Prepare Filter for this Association",  NULL, NULL, G_CALLBACK(sctp_set_assoc_filter) },


  { "/Follow TCP Stream",                           NULL,       "Follow TCP Stream",                    NULL, NULL, G_CALLBACK(follow_tcp_stream_cb) },
  { "/Follow UDP Stream",                           NULL,       "Follow UDP Stream",                    NULL, NULL, G_CALLBACK(follow_udp_stream_cb) },
  { "/Follow SSL Stream",                           NULL,       "Follow SSL Stream",                    NULL, NULL, G_CALLBACK(follow_ssl_stream_cb) },
  { "/Follow HTTP Stream",                          NULL,       "Follow HTTP Stream",                   NULL, NULL, G_CALLBACK(follow_http_stream_cb) },

  { "/Copy",        NULL, "Copy",                   NULL, NULL, NULL },
  { "/Copy/SummaryTxt",                             NULL,       "Summary (Text)",                       NULL, NULL, G_CALLBACK(packet_list_menu_copy_sum_txt) },
  { "/Copy/SummaryCSV",                             NULL,       "Summary (CSV)",                        NULL, NULL, G_CALLBACK(packet_list_menu_copy_sum_csv) },
  { "/Copy/AsFilter",                               NULL,       "As Filter",                            NULL, NULL, G_CALLBACK(packet_list_menu_copy_as_flt) },


  { "/Copy/Bytes",                                  NULL,       "Bytes",                    NULL, NULL, NULL },
  { "/Copy/Bytes/OffsetHexText",                    NULL,       "Offset Hex Text",                      NULL, NULL, G_CALLBACK(packet_list_menu_copy_bytes_oht_cb) },
  { "/Copy/Bytes/OffsetHex",                        NULL,       "Offset Hex",                           NULL, NULL, G_CALLBACK(packet_list_menu_copy_bytes_oh_cb) },
  { "/Copy/Bytes/PrintableTextOnly",                NULL,       "Printable Text Only",                  NULL, NULL, G_CALLBACK(packet_list_menu_copy_bytes_text_cb) },

  { "/Copy/Bytes/HexStream",                        NULL,       "Hex Stream",                           NULL, NULL, G_CALLBACK(packet_list_menu_copy_bytes_hex_strm_cb) },
  { "/Copy/Bytes/BinaryStream",                     NULL,       "Binary Stream",                        NULL, NULL, G_CALLBACK(packet_list_menu_copy_bytes_bin_strm_cb) },

  { "/ProtocolPreferences",                         NULL,       "Protocol Preferences",                 NULL, NULL, NULL },
  { "/DecodeAs",                                    WIRESHARK_STOCK_DECODE_AS,  "Decode As...",         NULL, NULL, G_CALLBACK(decode_as_cb) },
  { "/Print",                                       GTK_STOCK_PRINT,            "Print...",             NULL, NULL, G_CALLBACK(file_print_selected_cmd_cb) },
  { "/ShowPacketinNewWindow",                       NULL,           "Show Packet in New Window",        NULL, NULL, G_CALLBACK(new_window_cb) },

};

static const char *ui_desc_tree_view_menu_popup =
"<ui>\n"
"  <popup name='TreeViewPopup' action='PopupAction'>\n"
"     <menuitem name='ExpandSubtrees' action='/ExpandSubtrees'/>\n"
"     <menuitem name='CollapseSubtrees' action='/CollapseSubtrees'/>\n"
"     <menuitem name='ExpandAll' action='/ExpandAll'/>\n"
"     <menuitem name='CollapseAll' action='/CollapseAll'/>\n"
"     <separator/>\n"
"     <menuitem name='ApplyasColumn' action='/Apply as Column'/>\n"
"     <separator/>\n"
"     <menu name= 'ApplyAsFilter' action='/ApplyasFilter'>\n"
"       <menuitem name='Selected' action='/ApplyasFilter/Selected'/>\n"
"       <menuitem name='NotSelected' action='/ApplyasFilter/Not Selected'/>\n"
"       <menuitem name='AndSelected' action='/ApplyasFilter/AndSelected'/>\n"
"       <menuitem name='OrSelected' action='/ApplyasFilter/OrSelected'/>\n"
"       <menuitem name='AndNotSelected' action='/ApplyasFilter/AndNotSelected'/>\n"
"       <menuitem name='OrNotSelected' action='/ApplyasFilter/OrNotSelected'/>\n"
"     </menu>\n"
"     <menu name= 'PrepareaFilter' action='/PrepareaFilter'>\n"
"       <menuitem name='Selected' action='/PrepareaFilter/Selected'/>\n"
"       <menuitem name='NotSelected' action='/PrepareaFilter/Not Selected'/>\n"
"       <menuitem name='AndSelected' action='/PrepareaFilter/AndSelected'/>\n"
"       <menuitem name='OrSelected' action='/PrepareaFilter/OrSelected'/>\n"
"       <menuitem name='AndNotSelected' action='/PrepareaFilter/AndNotSelected'/>\n"
"       <menuitem name='OrNotSelected' action='/PrepareaFilter/OrNotSelected'/>\n"
"     </menu>\n"
"     <menu name= 'ColorizewithFilter' action='/Colorize with Filter'>\n"
"       <menuitem name='Color1' action='/Colorize with Filter/Color 1'/>\n"
"       <menuitem name='Color2' action='/Colorize with Filter/Color 2'/>\n"
"       <menuitem name='Color3' action='/Colorize with Filter/Color 3'/>\n"
"       <menuitem name='Color4' action='/Colorize with Filter/Color 4'/>\n"
"       <menuitem name='Color5' action='/Colorize with Filter/Color 5'/>\n"
"       <menuitem name='Color6' action='/Colorize with Filter/Color 6'/>\n"
"       <menuitem name='Color7' action='/Colorize with Filter/Color 7'/>\n"
"       <menuitem name='Color8' action='/Colorize with Filter/Color 8'/>\n"
"       <menuitem name='Color9' action='/Colorize with Filter/Color 9'/>\n"
"       <menuitem name='Color10' action='/Colorize with Filter/Color 10'/>\n"
"       <menuitem name='NewColoringRule' action='/Colorize with Filter/New Coloring Rule'/>\n"
"     </menu>\n"
"     <menuitem name='FollowTCPStream' action='/Follow TCP Stream'/>\n"
"     <menuitem name='FollowUDPStream' action='/Follow UDP Stream'/>\n"
"     <menuitem name='FollowSSLStream' action='/Follow SSL Stream'/>\n"
"     <menuitem name='FollowHTTPStream' action='/Follow HTTP Stream'/>\n"
"     <separator/>\n"
"     <menu name= 'Copy' action='/Copy'>\n"
"        <menuitem name='Description' action='/Copy/Description'/>\n"
"        <menuitem name='Fieldname' action='/Copy/Fieldname'/>\n"
"        <menuitem name='Value' action='/Copy/Value'/>\n"
"        <separator/>\n"
"        <menuitem name='AsFilter' action='/Copy/AsFilter'/>\n"
"        <separator/>\n"
"        <menu name= 'Bytes' action='/Copy/Bytes'>\n"
"           <menuitem name='OffsetHexText' action='/Copy/Bytes/OffsetHexText'/>\n"
"           <menuitem name='OffsetHex' action='/Copy/Bytes/OffsetHex'/>\n"
"           <menuitem name='PrintableTextOnly' action='/Copy/Bytes/PrintableTextOnly'/>\n"
"           <separator/>\n"
"           <menuitem name='HexStream' action='/Copy/Bytes/HexStream'/>\n"
"           <menuitem name='BinaryStream' action='/Copy/Bytes/BinaryStream'/>\n"
"        </menu>\n"
"     </menu>\n"
"     <menuitem name='ExportSelectedPacketBytes' action='/ExportSelectedPacketBytes'/>\n"
#ifdef WANT_PACKET_EDITOR
"     <menuitem name='EditPacket' action='/Edit/EditPacket'/>\n"
#endif
"     <separator/>\n"
"     <menuitem name='WikiProtocolPage' action='/WikiProtocolPage'/>\n"
"     <menuitem name='FilterFieldReference' action='/FilterFieldReference'/>\n"
"     <menuitem name='ProtocolHelp' action='/ProtocolHelp'/>\n"
"     <menuitem name='ProtocolPreferences' action='/ProtocolPreferences'/>\n"
"     <separator/>\n"
"     <menuitem name='DecodeAs' action='/DecodeAs'/>\n"
"     <menuitem name='DisableProtocol' action='/DisableProtocol'/>\n"
"     <menuitem name='ResolveName' action='/ResolveName'/>\n"
"     <menuitem name='GotoCorrespondingPacket' action='/GotoCorrespondingPacket'/>\n"
"     <menuitem name='ShowPacketRefinNewWindow' action='/ShowPacketRefinNewWindow'/>\n"
"  </popup>\n"
"</ui>\n";

static const GtkActionEntry tree_view_menu_popup_action_entries[] = {
  { "/ExpandSubtrees",                  NULL,                           "Expand Subtrees",         NULL,                   NULL,           G_CALLBACK(expand_tree_cb) },
  { "/CollapseSubtrees",                NULL,                           "Collapse Subtrees",       NULL,                   NULL,           G_CALLBACK(collapse_tree_cb) },
  { "/ExpandAll",                       NULL,                           "Expand All",              NULL,                   NULL,           G_CALLBACK(expand_all_cb) },
  { "/CollapseAll",                     NULL,                           "Collapse All",            NULL,                   NULL,           G_CALLBACK(collapse_all_cb) },
  { "/Apply as Column",                 NULL,                           "Apply as Column",         NULL,                   NULL,           G_CALLBACK(apply_as_custom_column_cb) },

  { "/Colorize with Filter",            NULL, "Colorize with Filter",   NULL, NULL, NULL },
  { "/Colorize with Filter/Color 1",        WIRESHARK_STOCK_COLOR1, "Color 1",                  NULL, NULL, G_CALLBACK(tree_view_menu_color_with_flt_color1_cb) },
  { "/Colorize with Filter/Color 2",        WIRESHARK_STOCK_COLOR2, "Color 2",                  NULL, NULL, G_CALLBACK(tree_view_menu_color_with_flt_color2_cb) },
  { "/Colorize with Filter/Color 3",        WIRESHARK_STOCK_COLOR3, "Color 3",                  NULL, NULL, G_CALLBACK(tree_view_menu_color_with_flt_color3_cb) },
  { "/Colorize with Filter/Color 4",        WIRESHARK_STOCK_COLOR4, "Color 4",                  NULL, NULL, G_CALLBACK(tree_view_menu_color_with_flt_color4_cb) },
  { "/Colorize with Filter/Color 5",        WIRESHARK_STOCK_COLOR5, "Color 5",                  NULL, NULL, G_CALLBACK(tree_view_menu_color_with_flt_color5_cb) },
  { "/Colorize with Filter/Color 6",        WIRESHARK_STOCK_COLOR6, "Color 6",                  NULL, NULL, G_CALLBACK(tree_view_menu_color_with_flt_color6_cb) },
  { "/Colorize with Filter/Color 7",        WIRESHARK_STOCK_COLOR7, "Color 7",                  NULL, NULL, G_CALLBACK(tree_view_menu_color_with_flt_color7_cb) },
  { "/Colorize with Filter/Color 8",        WIRESHARK_STOCK_COLOR8, "Color 8",                  NULL, NULL, G_CALLBACK(tree_view_menu_color_with_flt_color8_cb) },
  { "/Colorize with Filter/Color 9",        WIRESHARK_STOCK_COLOR9, "Color 9",                  NULL, NULL, G_CALLBACK(tree_view_menu_color_with_flt_color9_cb) },
  { "/Colorize with Filter/Color 10",       WIRESHARK_STOCK_COLOR0, "Color 10",                 NULL, NULL, G_CALLBACK(tree_view_menu_color_with_flt_color10_cb) },
  { "/Colorize with Filter/New Coloring Rule",  NULL,       "New Coloring Rule...",             NULL, NULL, G_CALLBACK(tree_view_menu_color_with_flt_new_rule_cb) },

  { "/Follow TCP Stream",                           NULL,       "Follow TCP Stream",                    NULL, NULL, G_CALLBACK(follow_tcp_stream_cb) },
  { "/Follow UDP Stream",                           NULL,       "Follow UDP Stream",                    NULL, NULL, G_CALLBACK(follow_udp_stream_cb) },
  { "/Follow SSL Stream",                           NULL,       "Follow SSL Stream",                    NULL, NULL, G_CALLBACK(follow_ssl_stream_cb) },
  { "/Follow HTTP Stream",                          NULL,       "Follow HTTP Stream",                   NULL, NULL, G_CALLBACK(follow_http_stream_cb) },

  { "/Copy",        NULL, "Copy",                   NULL, NULL, NULL },
  { "/Copy/Description",                            NULL,       "Description",                      NULL, NULL, G_CALLBACK(tree_view_menu_copy_desc) },
  { "/Copy/Fieldname",                              NULL,       "Fieldname",                        NULL, NULL, G_CALLBACK(tree_view_menu_copy_field) },
  { "/Copy/Value",                                  NULL,       "Value",                            NULL, NULL, G_CALLBACK(tree_view_menu_copy_value) },

  { "/Copy/AsFilter",                               NULL,       "As Filter",                        NULL, NULL, G_CALLBACK(tree_view_menu_copy_as_flt) },

  { "/Copy/Bytes",                                  NULL,       "Bytes",                                NULL, NULL, NULL },
  { "/Copy/Bytes/OffsetHexText",                    NULL,       "Offset Hex Text",                      NULL, NULL, G_CALLBACK(packet_list_menu_copy_bytes_oht_cb) },
  { "/Copy/Bytes/OffsetHex",                        NULL,       "Offset Hex",                           NULL, NULL, G_CALLBACK(packet_list_menu_copy_bytes_oh_cb) },
  { "/Copy/Bytes/PrintableTextOnly",                NULL,       "Printable Text Only",                  NULL, NULL, G_CALLBACK(packet_list_menu_copy_bytes_text_cb) },

  { "/Copy/Bytes/HexStream",                        NULL,       "Hex Stream",                           NULL, NULL, G_CALLBACK(packet_list_menu_copy_bytes_hex_strm_cb) },
  { "/Copy/Bytes/BinaryStream",                     NULL,       "Binary Stream",                        NULL, NULL, G_CALLBACK(packet_list_menu_copy_bytes_bin_strm_cb) },

  { "/ExportSelectedPacketBytes",                   NULL,       "Export Selected Packet Bytes...",      NULL, NULL, G_CALLBACK(savehex_cb) },
#ifdef WANT_PACKET_EDITOR
  { "/Edit/EditPacket",                NULL,               "_Edit Packet",                         NULL,                       NULL,           G_CALLBACK(edit_window_cb) },
#endif
  { "/WikiProtocolPage",            WIRESHARK_STOCK_WIKI,       "Wiki Protocol Page",                   NULL, NULL, G_CALLBACK(selected_ptree_info_cb) },
  { "/FilterFieldReference",    WIRESHARK_STOCK_INTERNET,       "Filter Field Reference",               NULL, NULL, G_CALLBACK(selected_ptree_ref_cb) },
  { "/ProtocolHelp",                                NULL,       "Protocol Help",                        NULL, NULL, NULL },
  { "/ProtocolPreferences",                         NULL,       "Protocol Preferences",                 NULL, NULL, NULL },
  { "/DecodeAs",                WIRESHARK_STOCK_DECODE_AS,      "Decode As...",                         NULL, NULL, G_CALLBACK(decode_as_cb) },
  { "/DisableProtocol",         WIRESHARK_STOCK_CHECKBOX,       "Disable Protocol...",                  NULL, NULL, G_CALLBACK(proto_disable_cb) },
  { "/ResolveName",                                 NULL,       "_Resolve Name",                        NULL, NULL, G_CALLBACK(resolve_name_cb) },
  { "/GotoCorrespondingPacket",                     NULL,       "_Go to Corresponding Packet",          NULL, NULL, G_CALLBACK(goto_framenum_cb) },
  { "/ShowPacketRefinNewWindow",                    NULL,       "Show Packet Reference in New Window",  NULL, NULL, G_CALLBACK(new_window_cb_ref) },
};

static const char *ui_desc_bytes_menu_popup =
"<ui>\n"
"  <popup name='BytesMenuPopup' action='PopupAction'>\n"
"     <menuitem name='HexView' action='/HexView'/>\n"
"     <menuitem name='BitsView' action='/BitsView'/>\n"
"  </popup>\n"
"</ui>\n";

static const GtkRadioActionEntry bytes_menu_radio_action_entries [] =
{
    /* name,    stock id,        label,      accel,  tooltip,  value */
    { "/HexView",   NULL,       "Hex View",   NULL,   NULL,     BYTES_HEX },
    { "/BitsView",  NULL,       "Bits View",  NULL,   NULL,     BYTES_BITS },
};

static const char *ui_statusbar_profiles_menu_popup =
"<ui>\n"
"  <popup name='ProfilesMenuPopup' action='PopupAction'>\n"
"     <menuitem name='Profiles' action='/Profiles'/>\n"
"     <separator/>\n"
"     <menuitem name='New' action='/New'/>\n"
"     <menuitem name='Rename' action='/Rename'/>\n"
"     <menuitem name='Delete' action='/Delete'/>\n"
"     <separator/>\n"
"     <menu name='Change' action='/Change'>\n"
"        <menuitem name='Default' action='/Change/Default'/>\n"
"     </menu>\n"
"  </popup>\n"
"</ui>\n";
static const GtkActionEntry statusbar_profiles_menu_action_entries [] =
{
    { "/Profiles",       GTK_STOCK_INDEX,   "Manage Profiles...", NULL, NULL, G_CALLBACK(profile_dialog_cb) },
    { "/New",            GTK_STOCK_NEW,     "New...",             NULL, NULL, G_CALLBACK(profile_new_cb) },
    { "/Rename",         GTK_STOCK_EDIT,    "Rename...",          NULL, NULL, G_CALLBACK(profile_rename_cb) },
    { "/Delete",         GTK_STOCK_DELETE,  "Delete",             NULL, NULL, G_CALLBACK(profile_delete_cb) },
    { "/Change",         GTK_STOCK_JUMP_TO, "Switch to",          NULL, NULL, NULL },
    { "/Change/Default", NULL,              "Default",            NULL, NULL, NULL },
};

GtkWidget *
main_menu_new(GtkAccelGroup ** table)
{
    GtkWidget *menubar;
#ifdef HAVE_IGE_MAC_INTEGRATION
    GtkWidget *quit_item, *about_item, *preferences_item;
    IgeMacMenuGroup *group;
#endif
#ifdef HAVE_GTKOSXAPPLICATION
    GtkosxApplication *theApp;
    GtkWidget * item;
    GtkWidget * dock_menu;
#endif

    grp = gtk_accel_group_new();

    if (initialize)
        menus_init();

    menubar = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar");
#ifdef HAVE_IGE_MAC_INTEGRATION
    if(prefs.gui_macosx_style) {
        ige_mac_menu_set_menu_bar(GTK_MENU_SHELL(menubar));
        ige_mac_menu_set_global_key_handler_enabled(TRUE);

        /* Create menu items to populate the application menu with.  We have to
         * do this because we are still using the old GtkItemFactory API for
         * the main menu. */
        group = ige_mac_menu_add_app_menu_group();
        about_item = gtk_menu_item_new_with_label("About");
        g_signal_connect(about_item, "activate", G_CALLBACK(about_wireshark_cb),
                         NULL);
        ige_mac_menu_add_app_menu_item(group, GTK_MENU_ITEM(about_item), NULL);

        group = ige_mac_menu_add_app_menu_group();
        preferences_item = gtk_menu_item_new_with_label("Preferences");
        g_signal_connect(preferences_item, "activate", G_CALLBACK(prefs_cb),
                         NULL);
        ige_mac_menu_add_app_menu_item(group, GTK_MENU_ITEM(preferences_item),
                                       NULL);
    }

    /* The quit item in the application menu shows up whenever ige mac
     * integration is enabled, even if the OS X UI style in Wireshark isn't
     * turned on. */
    quit_item = gtk_menu_item_new_with_label("Quit");
    g_signal_connect(quit_item, "activate", G_CALLBACK(file_quit_cmd_cb), NULL);
    ige_mac_menu_set_quit_menu_item(GTK_MENU_ITEM(quit_item));
#endif

#ifdef HAVE_GTKOSXAPPLICATION
    theApp = (GtkosxApplication *)g_object_new(GTKOSX_TYPE_APPLICATION, NULL);

    if(prefs.gui_macosx_style) {
        gtk_widget_hide(menubar);

        gtkosx_application_set_menu_bar(theApp, GTK_MENU_SHELL(menubar));
        gtkosx_application_set_use_quartz_accelerators(theApp, TRUE);

        /* Construct a conventional looking OSX App menu */

        item = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/HelpMenu/AboutWireshark");
        gtkosx_application_insert_app_menu_item(theApp, item, 0);

        gtkosx_application_insert_app_menu_item(theApp, gtk_separator_menu_item_new(), 1);

        item = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/EditMenu/Preferences");
        gtkosx_application_insert_app_menu_item(theApp, item, 2);

        /* Set OSX help menu */

        item = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/HelpMenu");
        gtkosx_application_set_help_menu(theApp,GTK_MENU_ITEM(item));

        /* Hide the File menu Quit item (a Quit item is automagically placed within the OSX App menu) */

        item = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/FileMenu/Quit");
        gtk_widget_hide(item);
    }

    /* generate dock menu */
    dock_menu = gtk_menu_new();

    item = gtk_menu_item_new_with_label("Start");
    g_signal_connect(item, "activate", G_CALLBACK (capture_start_cb), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(dock_menu), item);

    item = gtk_menu_item_new_with_label("Stop");
    g_signal_connect(item, "activate", G_CALLBACK (capture_stop_cb), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(dock_menu), item);

    item = gtk_menu_item_new_with_label("Restart");
    g_signal_connect(item, "activate", G_CALLBACK (capture_restart_cb), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(dock_menu), item);

    gtkosx_application_set_dock_menu(theApp, GTK_MENU_SHELL(dock_menu));
#endif

    if (table)
        *table = grp;

    plugin_if_register_gui_cb(PLUGIN_IF_PREFERENCE_SAVE, plugin_if_menubar_preference);

    return menubar;
}

static void
menu_dissector_filter_cb(GtkAction *action _U_,  gpointer callback_data)
{
    conversation_filter_t  *filter_entry = (conversation_filter_t *)callback_data;
    GtkWidget               *filter_te;
    const char              *buf;

    filter_te = gtk_bin_get_child(GTK_BIN(g_object_get_data(G_OBJECT(top_level), E_DFILTER_CM_KEY)));

    /* XXX - this gets the packet_info of the last dissected packet, */
    /* which is not necessarily the last selected packet */
    /* e.g. "Update list of packets in real time" won't work correct */
    buf = filter_entry->build_filter_string(&cfile.edt->pi);

    gtk_entry_set_text(GTK_ENTRY(filter_te), buf);

    /* Run the display filter so it goes in effect - even if it's the
       same as the previous display filter. */
    main_filter_packets(&cfile, buf, TRUE);

    g_free( (void *) buf);
}

static gboolean
menu_dissector_filter_spe_cb(frame_data *fd _U_, epan_dissect_t *edt, gpointer callback_data)
{
    conversation_filter_t *filter_entry = (conversation_filter_t*)callback_data;

    /* XXX - this gets the packet_info of the last dissected packet, */
    /* which is not necessarily the last selected packet */
    /* e.g. "Update list of packets in real time" won't work correct */
    return (edt != NULL) ? filter_entry->is_filter_valid(&edt->pi) : FALSE;
}

static void
menu_dissector_filter(capture_file *cf)
{
    GList *list_entry = conv_filter_list;
    conversation_filter_t *filter_entry;

    guint merge_id;
    GtkActionGroup *action_group;
    GtkAction *action;
    GtkWidget *submenu_dissector_filters;
    gchar *action_name;
    guint i = 0;


    merge_id = gtk_ui_manager_new_merge_id (ui_manager_main_menubar);

    action_group = gtk_action_group_new ("dissector-filters-group");

    submenu_dissector_filters = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/AnalyzeMenu/ConversationFilterMenu");
    if(!submenu_dissector_filters){
        g_warning("menu_dissector_filter: No submenu_dissector_filters found, path= /Menubar/AnalyzeMenu/ConversationFilterMenu");
    }

    gtk_ui_manager_insert_action_group (ui_manager_main_menubar, action_group, 0);
    g_object_set_data (G_OBJECT (ui_manager_main_menubar),
                     "dissector-filters-merge-id", GUINT_TO_POINTER (merge_id));

    /* no items */
    if (!list_entry){

      action = (GtkAction *)g_object_new (GTK_TYPE_ACTION,
                 "name", "filter-list-empty",
                 "label", "No filters",
                 "sensitive", FALSE,
                 NULL);
      gtk_action_group_add_action (action_group, action);
      gtk_action_set_sensitive(action, FALSE);
      g_object_unref (action);

      gtk_ui_manager_add_ui (ui_manager_main_menubar, merge_id,
                 "/Menubar/AnalyzeMenu/ConversationFilterMenu/Filters",
                 "filter-list-empty",
                 "filter-list-empty",
                 GTK_UI_MANAGER_MENUITEM,
                 FALSE);

      return;
    }

    while (list_entry != NULL) {
        filter_entry = (conversation_filter_t *)list_entry->data;
        action_name = g_strdup_printf ("filter-%u", i);
        /*g_warning("action_name %s, filter_entry->name %s",action_name,filter_entry->name);*/
        action = (GtkAction *)g_object_new (GTK_TYPE_ACTION,
                 "name", action_name,
                 "label", filter_entry->display_name,
                 "sensitive", menu_dissector_filter_spe_cb(/* frame_data *fd _U_*/ NULL, cf->edt, filter_entry),
                 NULL);
        g_signal_connect (action, "activate",
                        G_CALLBACK (menu_dissector_filter_cb), filter_entry);
        gtk_action_group_add_action (action_group, action);
        g_object_unref (action);

        gtk_ui_manager_add_ui (ui_manager_main_menubar, merge_id,
                 "/Menubar/AnalyzeMenu/ConversationFilterMenu/Filters",
                 action_name,
                 action_name,
                 GTK_UI_MANAGER_MENUITEM,
                 FALSE);
        g_free(action_name);
        i++;
        list_entry = g_list_next(list_entry);
    }
}

static void
menu_endpoints_cb(GtkAction *action _U_, gpointer user_data)
{
    register_ct_t *table = (register_ct_t*)user_data;

    conversation_endpoint_cb(table);
}

typedef struct {
    capture_file *cf;
    guint merge_id;
    GtkActionGroup *action_group;
    int counter;
} conv_menu_t;

static void
add_conversation_menuitem(gpointer data, gpointer user_data)
{
    register_ct_t *table = (register_ct_t*)data;
    conv_menu_t *conv = (conv_menu_t*)user_data;
    gchar *action_name;
    GtkAction *action;

    action_name = g_strdup_printf ("conversation-%u", conv->counter);
    /*g_warning("action_name %s, filter_entry->name %s",action_name,filter_entry->name);*/
    action = (GtkAction *)g_object_new (GTK_TYPE_ACTION,
                "name", action_name,
                "label", proto_get_protocol_short_name(find_protocol_by_id(get_conversation_proto_id(table))),
                "sensitive", TRUE,
                NULL);
    g_signal_connect (action, "activate",
                    G_CALLBACK (menu_endpoints_cb), table);
    gtk_action_group_add_action (conv->action_group, action);
    g_object_unref (action);

    gtk_ui_manager_add_ui (ui_manager_main_menubar, conv->merge_id,
                "/Menubar/StatisticsMenu/ConversationListMenu/Conversations",
                action_name,
                action_name,
                GTK_UI_MANAGER_MENUITEM,
                FALSE);
    g_free(action_name);
    conv->counter++;
}

static void
menu_conversation_list(capture_file *cf)
{
    GtkWidget *submenu_conversation_list;
    conv_menu_t conv_data;

    conv_data.merge_id = gtk_ui_manager_new_merge_id (ui_manager_main_menubar);

    conv_data.action_group = gtk_action_group_new ("conversation-list-group");

    submenu_conversation_list = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/StatisticsMenu/ConversationListMenu");
    if(!submenu_conversation_list){
        g_warning("menu_conversation_list: No submenu_conversation_list found, path= /Menubar/StatisticsMenu/ConversationListMenu");
    }

    gtk_ui_manager_insert_action_group (ui_manager_main_menubar, conv_data.action_group, 0);
    g_object_set_data (G_OBJECT (ui_manager_main_menubar),
                     "conversation-list-merge-id", GUINT_TO_POINTER (conv_data.merge_id));

    conv_data.cf = cf;
    conv_data.counter = 0;
    conversation_table_iterate_tables(add_conversation_menuitem, &conv_data);
}

static void
menu_hostlist_cb(GtkAction *action _U_, gpointer user_data)
{
    register_ct_t *table = (register_ct_t*)user_data;

    hostlist_endpoint_cb(table);
}

static void
add_hostlist_menuitem(gpointer data, gpointer user_data)
{
    register_ct_t *table = (register_ct_t*)data;
    conv_menu_t *conv = (conv_menu_t*)user_data;
    gchar *action_name;
    GtkAction *action;

    action_name = g_strdup_printf ("hostlist-%u", conv->counter);
    /*g_warning("action_name %s, filter_entry->name %s",action_name,filter_entry->name);*/
    action = (GtkAction *)g_object_new (GTK_TYPE_ACTION,
                "name", action_name,
                "label", proto_get_protocol_short_name(find_protocol_by_id(get_conversation_proto_id(table))),
                "sensitive", TRUE,
                NULL);
    g_signal_connect (action, "activate",
                    G_CALLBACK (menu_hostlist_cb), table);
    gtk_action_group_add_action (conv->action_group, action);
    g_object_unref (action);

    gtk_ui_manager_add_ui (ui_manager_main_menubar, conv->merge_id,
                "/Menubar/StatisticsMenu/EndpointListMenu/Endpoints",
                action_name,
                action_name,
                GTK_UI_MANAGER_MENUITEM,
                FALSE);
    g_free(action_name);
    conv->counter++;
}

static void
menu_hostlist_list(capture_file *cf)
{
    GtkWidget *submenu_hostlist;
    conv_menu_t conv_data;

    conv_data.merge_id = gtk_ui_manager_new_merge_id (ui_manager_main_menubar);

    conv_data.action_group = gtk_action_group_new ("endpoint-list-group");

    submenu_hostlist = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/StatisticsMenu/EndpointListMenu");
    if(!submenu_hostlist){
        g_warning("menu_hostlist_list: No submenu_conversation_list found, path= /Menubar/StatisticsMenu/EndpointListMenu");
    }

    gtk_ui_manager_insert_action_group (ui_manager_main_menubar, conv_data.action_group, 0);
    g_object_set_data (G_OBJECT (ui_manager_main_menubar),
                     "endpoint-list-merge-id", GUINT_TO_POINTER (conv_data.merge_id));

    conv_data.cf = cf;
    conv_data.counter = 0;
    conversation_table_iterate_tables(add_hostlist_menuitem, &conv_data);
}

static void
menu_conversation_display_filter_cb(GtkAction *action _U_, gpointer data)
{
    conversation_filter_t *filter_entry = (conversation_filter_t *)data;

    gchar     *filter;
    GtkWidget *filter_te;

    if (cfile.current_frame) {
        /* create a filter-string based on the selected packet and action */
        filter = filter_entry->build_filter_string(&cfile.edt->pi);

        /* Run the display filter so it goes in effect - even if it's the
        same as the previous display filter. */
        filter_te = gtk_bin_get_child(GTK_BIN(g_object_get_data(G_OBJECT(top_level), E_DFILTER_CM_KEY)));

        gtk_entry_set_text(GTK_ENTRY(filter_te), filter);
        main_filter_packets(&cfile, filter, TRUE);

        g_free(filter);
    }
}

static gboolean
menu_color_dissector_filter_spe_cb(frame_data *fd _U_, epan_dissect_t *edt, gpointer callback_data)
{
    conversation_filter_t *filter_entry = (conversation_filter_t *)callback_data;

    /* XXX - this gets the packet_info of the last dissected packet, */
    /* which is not necessarily the last selected packet */
    /* e.g. "Update list of packets in real time" won't work correct */
    return (edt != NULL) ? filter_entry->is_filter_valid(&edt->pi) : FALSE;
}

#define MAX_NUM_COLOR_CONVERSATION_COLORS       10

static void
menu_color_conversation_filter(capture_file *cf)
{
    GtkWidget *submenu_conv_filters, *submenu_color_conv_filters;
    guint merge_id, color_merge_id;
    GtkActionGroup *action_group, *color_action_group;
    GList *list_entry = conv_filter_list;
    conversation_filter_t* color_filter;
    int conv_counter = 0;

    static packet_list_menu_color_conv_color_cb_t callbacks[MAX_NUM_COLOR_CONVERSATION_COLORS] = {
        packet_list_menu_color_conv_color1_cb,
        packet_list_menu_color_conv_color2_cb,
        packet_list_menu_color_conv_color3_cb,
        packet_list_menu_color_conv_color4_cb,
        packet_list_menu_color_conv_color5_cb,
        packet_list_menu_color_conv_color6_cb,
        packet_list_menu_color_conv_color7_cb,
        packet_list_menu_color_conv_color8_cb,
        packet_list_menu_color_conv_color9_cb,
        packet_list_menu_color_conv_color10_cb,
    };

    static const gchar *icons[MAX_NUM_COLOR_CONVERSATION_COLORS] = {
        WIRESHARK_STOCK_COLOR1,
        WIRESHARK_STOCK_COLOR2,
        WIRESHARK_STOCK_COLOR3,
        WIRESHARK_STOCK_COLOR4,
        WIRESHARK_STOCK_COLOR5,
        WIRESHARK_STOCK_COLOR6,
        WIRESHARK_STOCK_COLOR7,
        WIRESHARK_STOCK_COLOR8,
        WIRESHARK_STOCK_COLOR9,
        WIRESHARK_STOCK_COLOR0,
    };

    merge_id = gtk_ui_manager_new_merge_id (ui_manager_packet_list_menu);
    action_group = gtk_action_group_new ("popup-conversation-filters-group");
    color_merge_id = gtk_ui_manager_new_merge_id (ui_manager_packet_list_menu);
    color_action_group = gtk_action_group_new ("popup-conv-color-filters-group");

    submenu_conv_filters = gtk_ui_manager_get_widget(ui_manager_packet_list_menu, "/PacketListMenuPopup/ConversationFilter");
    if(!submenu_conv_filters){
        g_warning("menu_color_conversation_filter: No submenu_conversation_filters found, path= /PacketListMenuPopup/ConversationFilter");
    }

    submenu_color_conv_filters = gtk_ui_manager_get_widget(ui_manager_packet_list_menu, "/PacketListMenuPopup/ColorizeConversation");
    if(!submenu_color_conv_filters){
        g_warning("menu_color_conversation_filter: No submenu_color_conversation_filters found, path= /PacketListMenuPopup/ColorizeConversation");
    }

    gtk_ui_manager_insert_action_group (ui_manager_packet_list_menu, action_group, 0);
    g_object_set_data (G_OBJECT (ui_manager_packet_list_menu),
                     "popup-conversation-filters-merge-id", GUINT_TO_POINTER (merge_id));
    gtk_ui_manager_insert_action_group (ui_manager_packet_list_menu, color_action_group, 0);
    g_object_set_data (G_OBJECT (ui_manager_packet_list_menu),
                     "popup-conv-color-filters-merge-id", GUINT_TO_POINTER (color_merge_id));

    while (list_entry != NULL) {
        gchar *action_name, *color_num_path_name;
        GtkAction *action, *color_action;
        GtkWidget *color_conv_filter_menuitem, *color_conv_filter_submenu, *color_conv_widget;

        color_filter = (conversation_filter_t*)list_entry->data;

        /* Create conversation filter menu item for each registered protocol */
        action_name = g_strdup_printf ("color_conversation-%u", conv_counter);
        conv_counter++;
        action = (GtkAction *)g_object_new (GTK_TYPE_ACTION,
                 "name", action_name,
                 "label", color_filter->display_name,
                 "sensitive", menu_color_dissector_filter_spe_cb(NULL, cf->edt, color_filter),
                 NULL);
        g_signal_connect (action, "activate", G_CALLBACK (menu_conversation_display_filter_cb), color_filter);
        gtk_action_group_add_action (action_group, action);
        g_object_unref (action);

        gtk_ui_manager_add_ui (ui_manager_packet_list_menu, merge_id,
                    "/PacketListMenuPopup/ConversationFilter/Conversations",
                    action_name,
                    action_name,
                    GTK_UI_MANAGER_MENUITEM,
                    FALSE);
        g_free(action_name);

        /* Create color filter menu item for each registered protocol */
        color_action = (GtkAction *)g_object_new (GTK_TYPE_ACTION,
                 "name", color_filter->display_name,
                 "label", color_filter->display_name,
                 "sensitive", menu_color_dissector_filter_spe_cb(NULL, cf->edt, color_filter),
                 NULL);
        gtk_action_group_add_action (color_action_group, color_action);
        g_object_unref (color_action);

        gtk_ui_manager_add_ui (ui_manager_packet_list_menu, color_merge_id,
                    "/PacketListMenuPopup/ColorizeConversation/Colorize",
                    color_filter->display_name,
                    color_filter->display_name,
                    GTK_UI_MANAGER_MENUITEM,
                    FALSE);


        /* Create each "numbered" color filter menu item for each registered protocol */
        color_num_path_name = g_strdup_printf ("/PacketListMenuPopup/ColorizeConversation/Colorize/%s", color_filter->display_name);
        color_conv_widget = gtk_ui_manager_get_widget(ui_manager_packet_list_menu, color_num_path_name);
        if (color_conv_widget != NULL) {
            guint i;
            gchar *color_num_name;

            color_conv_filter_submenu = gtk_menu_new();
            for (i = 0; i < MAX_NUM_COLOR_CONVERSATION_COLORS; i++) {
                color_num_name = g_strdup_printf ("Color %d", i+1);
                color_conv_filter_menuitem = gtk_image_menu_item_new_with_label(color_num_name);
                gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(color_conv_filter_menuitem),
                                       ws_gtk_image_new_from_stock(icons[i], GTK_ICON_SIZE_MENU));
                g_signal_connect(color_conv_filter_menuitem, "activate", G_CALLBACK(callbacks[i]), color_filter);
                gtk_menu_shell_append(GTK_MENU_SHELL(color_conv_filter_submenu), color_conv_filter_menuitem);

                gtk_widget_show (color_conv_filter_menuitem);
                g_free(color_num_name);
            }

            /* Create New Coloring Rule... menu item */
            color_conv_filter_menuitem = gtk_menu_item_new_with_label("New Coloring Rule...");
            g_signal_connect(color_conv_filter_menuitem, "activate", G_CALLBACK(packet_list_menu_color_conv_new_rule_cb), color_filter);
            gtk_menu_shell_append(GTK_MENU_SHELL(color_conv_filter_submenu), color_conv_filter_menuitem);
            gtk_widget_show (color_conv_filter_menuitem);

            gtk_menu_item_set_submenu (GTK_MENU_ITEM(color_conv_widget), color_conv_filter_submenu);
        } else {
            g_warning("menu_color_conversation_filter: No submenu_color_conv_filters found, path= %s", color_num_path_name);
        }
        g_free(color_num_path_name);

        list_entry = g_list_next(list_entry);
    }
}

static void
menus_init(void)
{
    GtkActionGroup *packet_list_heading_action_group, *packet_list_action_group,
        *packet_list_details_action_group, *packet_list_byte_menu_action_group,
        *statusbar_profiles_action_group;
    GtkAction *name_res_action;
    GError *error = NULL;
    guint merge_id;

    if (initialize) {
        initialize = FALSE;

        popup_menu_object = gtk_menu_new();

        /* packet list heading pop-up menu */
        packet_list_heading_action_group = gtk_action_group_new ("PacketListHeadingPopUpMenuActionGroup");

        gtk_action_group_add_actions (packet_list_heading_action_group,            /* the action group */
            packet_list_heading_menu_popup_action_entries,               /* an array of action descriptions */
            G_N_ELEMENTS(packet_list_heading_menu_popup_action_entries),           /* the number of entries */
            popup_menu_object);                                                    /* data to pass to the action callbacks */

        gtk_action_group_add_toggle_actions(packet_list_heading_action_group,                     /* the action group */
                                    (GtkToggleActionEntry *)packet_list_heading_menu_toggle_action_entries,     /* an array of action descriptions */
                                    G_N_ELEMENTS(packet_list_heading_menu_toggle_action_entries), /* the number of entries */
                                    NULL);                                                        /* data to pass to the action callbacks */

        ui_manager_packet_list_heading = gtk_ui_manager_new ();
        gtk_ui_manager_insert_action_group (ui_manager_packet_list_heading,
            packet_list_heading_action_group,
            0); /* the position at which the group will be inserted.  */

        gtk_ui_manager_add_ui_from_string (ui_manager_packet_list_heading,ui_desc_packet_list_heading_menu_popup, -1, &error);
        if (error != NULL)
        {
            fprintf (stderr, "Warning: building Packet List Heading Pop-Up failed: %s\n",
                    error->message);
            g_error_free (error);
            error = NULL;
        }

        g_object_set_data(G_OBJECT(popup_menu_object), PM_PACKET_LIST_COL_KEY,
                       gtk_ui_manager_get_widget(ui_manager_packet_list_heading, "/PacketListHeadingPopup"));

        popup_menu_list = g_slist_append((GSList *)popup_menu_list, ui_manager_packet_list_heading);

        /* packet list pop-up menu */
        packet_list_action_group = gtk_action_group_new ("PacketListPopUpMenuActionGroup");

        gtk_action_group_add_actions (packet_list_action_group,                    /* the action group */
            (GtkActionEntry *)packet_list_menu_popup_action_entries,                       /* an array of action descriptions */
            G_N_ELEMENTS(packet_list_menu_popup_action_entries),                   /* the number of entries */
            popup_menu_object);                                                    /* data to pass to the action callbacks */

        /* Add the filter menu items */
        gtk_action_group_add_actions (packet_list_action_group,                    /* the action group */
            (GtkActionEntry *)apply_prepare_filter_action_entries,                         /* an array of action descriptions */
            G_N_ELEMENTS(apply_prepare_filter_action_entries),                     /* the number of entries */
            popup_menu_object);                                                    /* data to pass to the action callbacks */

        ui_manager_packet_list_menu = gtk_ui_manager_new ();

        gtk_ui_manager_insert_action_group (ui_manager_packet_list_menu,
            packet_list_action_group,
            0); /* the position at which the group will be inserted.  */

        gtk_ui_manager_add_ui_from_string (ui_manager_packet_list_menu, ui_desc_packet_list_menu_popup, -1, &error);
        if (error != NULL)
        {
            fprintf (stderr, "Warning: building Packet List Pop-Up menu failed: %s\n",
                    error->message);
            g_error_free (error);
            error = NULL;
        }

        g_object_set_data(G_OBJECT(popup_menu_object), PM_PACKET_LIST_KEY,
                        gtk_ui_manager_get_widget(ui_manager_packet_list_menu, "/PacketListMenuPopup"));

        popup_menu_list = g_slist_append((GSList *)popup_menu_list, ui_manager_packet_list_menu);

        menu_color_conversation_filter(&cfile);

        /* packet detail pop-up menu */
        packet_list_details_action_group = gtk_action_group_new ("PacketListDetailsMenuPopUpActionGroup");

        gtk_action_group_add_actions (packet_list_details_action_group,            /* the action group */
            (GtkActionEntry *)tree_view_menu_popup_action_entries,                 /* an array of action descriptions */
            G_N_ELEMENTS(tree_view_menu_popup_action_entries),                     /* the number of entries */
            popup_menu_object);                                                    /* data to pass to the action callbacks */

        /* Add the filter menu items */
        gtk_action_group_add_actions (packet_list_details_action_group,            /* the action group */
            (GtkActionEntry *)apply_prepare_filter_action_entries,                 /* an array of action descriptions */
            G_N_ELEMENTS(apply_prepare_filter_action_entries),                     /* the number of entries */
            popup_menu_object);                                                    /* data to pass to the action callbacks */


        ui_manager_tree_view_menu = gtk_ui_manager_new ();

        gtk_ui_manager_insert_action_group (ui_manager_tree_view_menu,
            packet_list_details_action_group,
            0); /* the position at which the group will be inserted.  */
        gtk_ui_manager_add_ui_from_string (ui_manager_tree_view_menu, ui_desc_tree_view_menu_popup, -1, &error);
#if 0
        /* If we want to load the treewiew popup UI description from file */
        gui_desc_file_name_and_path = get_ui_file_path("tree-view-ui.xml");
        gtk_ui_manager_add_ui_from_file ( ui_manager_tree_view_menu, gui_desc_file_name_and_path, &error);
        g_free (gui_desc_file_name_and_path);
#endif
        if (error != NULL)
        {
            fprintf (stderr, "Warning: building TreeView Pop-Up menu failed: %s\n",
                    error->message);
            g_error_free (error);
            error = NULL;
        }

        g_object_set_data(G_OBJECT(popup_menu_object), PM_TREE_VIEW_KEY,
                         gtk_ui_manager_get_widget(ui_manager_tree_view_menu, "/TreeViewPopup"));

        popup_menu_list = g_slist_append((GSList *)popup_menu_list, ui_manager_tree_view_menu);

        /*
         * Hex dump pop-up menu.
         * We provide our own empty menu to suppress the default pop-up menu
         * for text widgets.
         */
        packet_list_byte_menu_action_group = gtk_action_group_new ("PacketListByteMenuPopUpActionGroup");


        gtk_action_group_add_radio_actions  (packet_list_byte_menu_action_group,            /* the action group */
                                    (GtkRadioActionEntry *)bytes_menu_radio_action_entries, /* an array of radio action descriptions  */
                                    G_N_ELEMENTS(bytes_menu_radio_action_entries),          /* the number of entries */
                                    recent.gui_bytes_view,                                  /* the value of the action to activate initially, or -1 if no action should be activated  */
                                    G_CALLBACK(select_bytes_view_cb),                       /* the callback to connect to the changed signal  */
                                    popup_menu_object);                                     /* data to pass to the action callbacks  */

        ui_manager_bytes_menu = gtk_ui_manager_new ();

        gtk_ui_manager_insert_action_group (ui_manager_bytes_menu,
            packet_list_byte_menu_action_group,
            0); /* the position at which the group will be inserted.  */
        gtk_ui_manager_add_ui_from_string (ui_manager_bytes_menu, ui_desc_bytes_menu_popup, -1, &error);
#if 0
        /* If we want to load the bytesview poupup UI description from file */
        gui_desc_file_name_and_path = get_ui_file_path("bytes-view-ui.xml");
        gtk_ui_manager_add_ui_from_file ( ui_manager_bytes_menu, gui_desc_file_name_and_path, &error);
        g_free (gui_desc_file_name_and_path);
#endif
        if (error != NULL)
        {
            fprintf (stderr, "Warning: building Bytes Pop-Up menu failed: %s\n",
                    error->message);
            g_error_free (error);
            error = NULL;
        }
        g_object_unref(packet_list_byte_menu_action_group);

        g_object_set_data(G_OBJECT(popup_menu_object), PM_BYTES_VIEW_KEY,
                        gtk_ui_manager_get_widget(ui_manager_bytes_menu, "/BytesMenuPopup"));

        popup_menu_list = g_slist_append((GSList *)popup_menu_list, ui_manager_bytes_menu);

        /* main */
        main_menu_bar_action_group = gtk_action_group_new ("MenuActionGroup");

        gtk_action_group_add_actions (main_menu_bar_action_group,                       /* the action group */
                                    main_menu_bar_entries,                              /* an array of action descriptions */
                                    G_N_ELEMENTS(main_menu_bar_entries),                /* the number of entries */
                                    NULL);                                              /* data to pass to the action callbacks */

#ifdef HAVE_LIBPCAP
        /* Add the capture menu actions */
        gtk_action_group_add_actions (main_menu_bar_action_group,                       /* the action group */
                                    capture_menu_entries,                               /* an array of action descriptions */
                                    G_N_ELEMENTS(capture_menu_entries),                 /* the number of entries */
                                    NULL);                                              /* data to pass to the action callbacks */
#endif

        /* Add the filter menu actions */
        gtk_action_group_add_actions (main_menu_bar_action_group,                       /* the action group */
                                    (GtkActionEntry *)apply_prepare_filter_action_entries,      /* an array of action descriptions */
                                    G_N_ELEMENTS(apply_prepare_filter_action_entries),  /* the number of entries */
                                    popup_menu_object);                                 /* data to pass to the action callbacks */

        gtk_action_group_add_toggle_actions(main_menu_bar_action_group,                 /* the action group */
                                    main_menu_bar_toggle_action_entries,                /* an array of action descriptions */
                                    G_N_ELEMENTS(main_menu_bar_toggle_action_entries),  /* the number of entries */
                                    NULL);                                              /* data to pass to the action callbacks */

        if (global_commandline_info.time_format != TS_NOT_SET) {
            recent.gui_time_format = global_commandline_info.time_format;
        }
        gtk_action_group_add_radio_actions  (main_menu_bar_action_group,                 /* the action group */
                                    main_menu_bar_radio_view_time_entries,               /* an array of radio action descriptions  */
                                    G_N_ELEMENTS(main_menu_bar_radio_view_time_entries), /* the number of entries */
                                    recent.gui_time_format,                              /* the value of the action to activate initially, or -1 if no action should be activated  */
                                    G_CALLBACK(timestamp_format_new_cb),                 /* the callback to connect to the changed signal  */
                                    NULL);                                               /* data to pass to the action callbacks  */

        gtk_action_group_add_radio_actions  (main_menu_bar_action_group,                                    /* the action group */
                                    main_menu_bar_radio_view_time_fileformat_prec_entries,                  /* an array of radio action descriptions  */
                                    G_N_ELEMENTS(main_menu_bar_radio_view_time_fileformat_prec_entries),    /* the number of entries */
                                    recent.gui_time_precision,                                /* the value of the action to activate initially, or -1 if no action should be activated  */
                                    G_CALLBACK(timestamp_precision_new_cb),                   /* the callback to connect to the changed signal  */
                                    NULL);                                                    /* data to pass to the action callbacks  */



        ui_manager_main_menubar = gtk_ui_manager_new ();
        gtk_ui_manager_insert_action_group (ui_manager_main_menubar, main_menu_bar_action_group, 0);

        gtk_ui_manager_add_ui_from_string (ui_manager_main_menubar,ui_desc_menubar, -1, &error);
        if (error != NULL)
        {
            fprintf (stderr, "Warning: building main menubar failed: %s\n",
                    error->message);
            g_error_free (error);
            error = NULL;
        }
        g_object_unref(main_menu_bar_action_group);
        gtk_window_add_accel_group (GTK_WINDOW(top_level),
                                gtk_ui_manager_get_accel_group(ui_manager_main_menubar));


        /* Add the recent files items to the menu
         * use place holders and
         * gtk_ui_manager_add_ui().
         */
        merge_id = gtk_ui_manager_new_merge_id (ui_manager_main_menubar);
        add_recent_items (merge_id, ui_manager_main_menubar);

        /* Add statistics tap plug-in to the menu
         */
        merge_id = gtk_ui_manager_new_merge_id (ui_manager_main_menubar);
        add_tap_plugins (merge_id, ui_manager_main_menubar);

        statusbar_profiles_action_group = gtk_action_group_new ("StatusBarProfilesPopUpMenuActionGroup");

        gtk_action_group_add_actions (statusbar_profiles_action_group,   /* the action group */
            (GtkActionEntry *)statusbar_profiles_menu_action_entries,    /* an array of action descriptions */
            G_N_ELEMENTS(statusbar_profiles_menu_action_entries),        /* the number of entries */
            popup_menu_object);                                          /* data to pass to the action callbacks */

        ui_manager_statusbar_profiles_menu = gtk_ui_manager_new ();
        gtk_ui_manager_insert_action_group (ui_manager_statusbar_profiles_menu,
            statusbar_profiles_action_group,
            0); /* the position at which the group will be inserted.  */

        gtk_ui_manager_add_ui_from_string (ui_manager_statusbar_profiles_menu,ui_statusbar_profiles_menu_popup, -1, &error);
        if (error != NULL)
        {
            fprintf (stderr, "Warning: building StatusBar Profiles Pop-Up failed: %s\n",
                    error->message);
            g_error_free (error);
            error = NULL;
        }

        g_object_unref(statusbar_profiles_action_group);

        g_object_set_data(G_OBJECT(popup_menu_object), PM_STATUSBAR_PROFILES_KEY,
                       gtk_ui_manager_get_widget(ui_manager_statusbar_profiles_menu, "/ProfilesMenuPopup"));

        popup_menu_list = g_slist_append((GSList *)popup_menu_list, ui_manager_statusbar_profiles_menu);

        menu_dissector_filter(&cfile);
        menu_conversation_list(&cfile);
        menu_hostlist_list(&cfile);

        /* Add additional entries which may have been introduced by dissectors and/or plugins */
        ws_menubar_external_menus();

        merge_menu_items(merge_menu_items_list);

        /* Add external menus and items */
        ws_menubar_build_external_menus();

        /* Initialize enabled/disabled state of menu items */
        set_menus_for_capture_file(NULL);
#if 0
        /* Un-#if this when we actually implement Cut/Copy/Paste.
           Then make sure you enable them when they can be done. */
        set_menu_sensitivity_old("/Edit/Cut", FALSE);
        set_menu_sensitivity_old("/Edit/Copy", FALSE);
        set_menu_sensitivity_old("/Edit/Paste", FALSE);
#endif
       /* Hide not usable menus */

        set_menus_for_captured_packets(FALSE);
        set_menus_for_selected_packet(&cfile);
        set_menus_for_selected_tree_row(&cfile);
        set_menus_for_capture_in_progress(FALSE);
        set_menus_for_file_set(/* dialog */TRUE, /* previous file */ FALSE, /* next_file */ FALSE);

        /* Set callback to update name resolution settings when activated */
        name_res_action = gtk_action_group_get_action(main_menu_bar_action_group, "/View/NameResolution");
        g_signal_connect (name_res_action, "activate", G_CALLBACK (menu_name_resolution_update_cb), NULL);
    }
}

/* Get a merge id for the menubar */
void
ws_add_build_menubar_items_callback(gpointer callback)
{
     build_menubar_items_callback_list = g_list_append(build_menubar_items_callback_list, callback);

}

static void
ws_menubar_build_external_menus(void)
{
    void (*callback)(gpointer);

    while (build_menubar_items_callback_list != NULL) {
        callback = (void (*)(gpointer))build_menubar_items_callback_list->data;
        callback(ui_manager_main_menubar);
        build_menubar_items_callback_list = g_list_next(build_menubar_items_callback_list);
    }


}

typedef struct _menu_item {
    const char   *gui_path;
    const char   *name;
    const char   *stock_id;
    const char   *label;
    const char   *accelerator;
    const gchar  *tooltip;
    GCallback    callback;
    gpointer     callback_data;
    gboolean     enabled;
    gboolean (*selected_packet_enabled)(frame_data *, epan_dissect_t *, gpointer callback_data);
    gboolean (*selected_tree_row_enabled)(field_info *, gpointer callback_data);
} menu_item_t;

static gint
insert_sorted_by_label(gconstpointer aparam, gconstpointer bparam)
{
    const menu_item_t *a = (menu_item_t *)aparam;
    const menu_item_t *b = (menu_item_t *)bparam;

    return g_ascii_strcasecmp(a->label, b->label);
}

void register_menu_bar_menu_items(
    const char   *gui_path,
    const char   *name,
    const gchar  *stock_id,
    const char   *label,
    const char   *accelerator,
    const gchar  *tooltip,
    gpointer     callback,
    gpointer     callback_data,
    gboolean     enabled,
    gboolean (*selected_packet_enabled)(frame_data *, epan_dissect_t *, gpointer callback_data),
    gboolean (*selected_tree_row_enabled)(field_info *, gpointer callback_data))
{
    menu_item_t *menu_item_data;

    menu_item_data                   = g_new0(menu_item_t,1);
    menu_item_data->gui_path         = gui_path;
    menu_item_data->name             = name;
    menu_item_data->label            = label;
    menu_item_data->stock_id         = stock_id;
    menu_item_data->accelerator      = accelerator;
    menu_item_data->tooltip          = tooltip;
    menu_item_data->callback         = (GCallback)callback;
    menu_item_data->callback_data    = callback_data;
    menu_item_data->enabled          = enabled;
    menu_item_data->selected_packet_enabled = selected_packet_enabled;
    menu_item_data->selected_tree_row_enabled = selected_tree_row_enabled;

    merge_menu_items_list = g_list_insert_sorted(merge_menu_items_list,
                                                 menu_item_data,
                                                 insert_sorted_by_label);
}

#define XMENU_MAX_DEPTH         (1 + 32)        /* max number of menus in an xpath (+1 for Menubar) */

static void
add_menu_item_to_main_menubar(const gchar *path, const gchar *name, const menu_item_t *menu_item_data)
{
    gchar           *xpath;
    GString         *item_path;
    guint            merge_id;
    char           **p;
    char           **tokens, **name_action_tokens;
    char            *tok, *item_name, *action_name;
    size_t           len;
    int              i;
    GtkAction       *action;

    /* no need to specify menu bar...skip it */
    len = strlen("/Menubar");
    if (g_ascii_strncasecmp(path, "/Menubar", len) == 0) {
        path += len;
    }

    xpath = g_strdup_printf("%s/%s", path, name);
    item_path = g_string_new("/Menubar");

    merge_id = gtk_ui_manager_new_merge_id(ui_manager_main_menubar);

    /*
     * The last item in the path is treated as the menu item; all preceding path
     * elements are the names of parent menus. Path elements are stripped of
     * leading/trailing spaces.
     *
     * |'s separate an existing menu's name from its action.
     * If the action has a / in it, it must have been "escaped" into a # before
     * entering this function; this function will translate it back to a /.
     * There must be an easier way!
     *
     * Examples:
     *
     *   "/Foo/Bar|/BarAction/I_tem" creates a hierarchy of:
     *
     *     A menu with an action of "Foo"
     *         A menu with a name of "Bar" and an action of "/BarAction"
     *             A menu item with an action of "I_tem", which puts
     *               the shortcut on "t".
     *
     *   "/Foo/Bar|BarAction/-/Baz|BarAction#BazAction/Item" creates
     *     a hierarchy of:
     *
     *     A menu with an action of "Foo"
     *         A menu with a name of "Bar" and an action of "/BarAction"
     *         A separator
     *             A menu with a name of "Baz" and an action of
     *               "BarAction/BazAction"
     *                 A menu item with an action of "Item"
     */
    tokens = g_strsplit(xpath, "/", XMENU_MAX_DEPTH);
    for (p = tokens; (p != NULL) && (*p != NULL); p++) {
        tok = g_strstrip(*p);
        if (g_strcmp0(tok, "-") == 0) {
            /* Just a separator. */
            gtk_ui_manager_add_ui(ui_manager_main_menubar, merge_id,
                                  item_path->str,
                                  NULL,
                                  NULL,
                                  GTK_UI_MANAGER_SEPARATOR,
                                  FALSE);
        } else {
            if (*(p+1) == NULL) {
                /*
                 * This is the last token; it's the name of a menu item,
                 * not of a menu.
                 *
                 * Allow a blank menu item name or else the item is hidden
                 * (and thus useless). Showing a blank menu item allows the
                 * developer to see the problem and fix it.
                 */
                item_name = tok;
                action_name = g_strconcat("/", tok, NULL);
                if (menu_item_data != NULL) {
                    action = (GtkAction *)g_object_new (
                            GTK_TYPE_ACTION,
                            "name", action_name,
                            "label", menu_item_data->label,
                            "stock-id", menu_item_data->stock_id,
                            "tooltip", menu_item_data->tooltip,
                            "sensitive", menu_item_data->enabled,
                            NULL
                    );
                    if (menu_item_data->callback != NULL) {
                        g_signal_connect (
                                action,
                                "activate",
                                G_CALLBACK (menu_item_data->callback),
                                menu_item_data->callback_data
                        );
                    }
                    gtk_action_group_add_action (main_menu_bar_action_group, action);
                    g_object_unref (action);
                }
                gtk_ui_manager_add_ui(ui_manager_main_menubar, merge_id,
                                      item_path->str,
                                      item_name,
                                      action_name,
                                      GTK_UI_MANAGER_MENUITEM,
                                      FALSE);
                g_free(action_name);
            } else {
                /*
                 * This is not the last token; it's the name of an
                 * intermediate menu.
                 *
                 * If it's empty, just skip it.
                 */
                if (tok[0] == '\0')
                    continue;

                /* Split the name of the menu from its action (if any) */
                name_action_tokens = g_strsplit(tok, "|", 2);
                if (name_action_tokens[1]) {
                    i = -1;
                    /* Replace #'s with /'s.
                     * Necessary for menus whose action includes a "/".
                     * There MUST be an easier way...
                     */
                    while (name_action_tokens[1][++i])
                        if (name_action_tokens[1][i] == '#')
                            name_action_tokens[1][i] = '/';
                    item_name = name_action_tokens[0];
                    action_name = g_strconcat("/", name_action_tokens[1], NULL);
                } else {
                    item_name = tok;
                    action_name = g_strconcat("/", tok, NULL);
                }

                if (menu_item_data != NULL) {
                    /*
                     * Add an action for this menu if it doesn't already
                     * exist.
                     */
                    if (gtk_action_group_get_action(main_menu_bar_action_group, action_name) == NULL) {
                        action = (GtkAction *)g_object_new (
                                GTK_TYPE_ACTION,
                                "name", action_name,
                                "label", item_name,
                                NULL
                        );
                        gtk_action_group_add_action (main_menu_bar_action_group, action);
                        g_object_unref (action);
                    }
                }
                gtk_ui_manager_add_ui(ui_manager_main_menubar, merge_id,
                                      item_path->str,
                                      item_name,
                                      action_name,
                                      GTK_UI_MANAGER_MENU,
                                      FALSE);
                g_free(action_name);

                g_string_append_printf(item_path, "/%s", item_name);
                g_strfreev(name_action_tokens);
            }
        }
    }

    /* we're finished processing the tokens so free the list */
    g_strfreev(tokens);

    g_string_free(item_path, TRUE);
    g_free(xpath);
}

static void
merge_menu_items(GList *lcl_merge_menu_items_list)
{
    menu_item_t    *menu_item_data;

    while (lcl_merge_menu_items_list != NULL) {
        menu_item_data = (menu_item_t *)lcl_merge_menu_items_list->data;
        add_menu_item_to_main_menubar(menu_item_data->gui_path, menu_item_data->name, menu_item_data);
        lcl_merge_menu_items_list = g_list_next(lcl_merge_menu_items_list);
    }
}

const char *
stat_group_name(register_stat_group_t group)
{
    /* See add_menu_item_to_main_menubar() for an explanation of the string format */
    static const value_string group_name_vals[] = {
        {REGISTER_ANALYZE_GROUP_UNSORTED,            "/Menubar/AnalyzeMenu|Analyze"},                                                              /* unsorted analyze stuff */
        {REGISTER_ANALYZE_GROUP_CONVERSATION_FILTER, "/Menubar/AnalyzeMenu|Analyze/ConversationFilterMenu|Analyze#ConversationFilter"},            /* conversation filters */
        {REGISTER_STAT_GROUP_UNSORTED,               "/Menubar/StatisticsMenu|Statistics"},                                                        /* unsorted statistic function */
        {REGISTER_STAT_GROUP_GENERIC,                "/Menubar/StatisticsMenu|Statistics"},                                                        /* generic statistic function, not specific to a protocol */
        {REGISTER_STAT_GROUP_CONVERSATION_LIST,      "/Menubar/StatisticsMenu|Statistics/ConversationListMenu|Statistics#ConversationList"},       /* member of the conversation list */
        {REGISTER_STAT_GROUP_ENDPOINT_LIST,          "/Menubar/StatisticsMenu|Statistics/EndpointListMenu|Statistics#EndpointList"},               /* member of the endpoint list */
        {REGISTER_STAT_GROUP_RESPONSE_TIME,          "/Menubar/StatisticsMenu|Statistics/ServiceResponseTimeMenu|Statistics#ServiceResponseTime"}, /* member of the service response time list */
        {REGISTER_STAT_GROUP_TELEPHONY,              "/Menubar/TelephonyMenu|Telephony"},                                                          /* telephony specific */
        {REGISTER_STAT_GROUP_TELEPHONY_ANSI,         "/Menubar/TelephonyMenu|Telephony/ANSI|Telephony#ANSI"},                                      /* ANSI-specific */
        {REGISTER_STAT_GROUP_TELEPHONY_GSM,          "/Menubar/TelephonyMenu|Telephony/GSM|Telephony#GSM"},                                        /* GSM-specific */
        {REGISTER_STAT_GROUP_TELEPHONY_LTE,          "/Menubar/TelephonyMenu|Telephony/LTEmenu|Telephony#LTE"},                                    /* LTE-specific */
        {REGISTER_STAT_GROUP_TELEPHONY_MTP3,         "/Menubar/TelephonyMenu|Telephony/MTP3menu|Telephony#MTP3"},                                  /* MTP3-specific */
        {REGISTER_STAT_GROUP_TELEPHONY_SCTP,         "/Menubar/TelephonyMenu|Telephony/SCTPmenu|Telephony#SCTP"},                                  /* SCTP-specific */
        {REGISTER_TOOLS_GROUP_UNSORTED,              "/Menubar/ToolsMenu|Tools"},                                                                  /* unsorted tools */
        {0, NULL}
    };
    return val_to_str_const(group, group_name_vals, "/Menubar/ToolsMenu|Tools");
}

/*
 * Enable/disable menu sensitivity.
 */
static void
set_menu_sensitivity(GtkUIManager *ui_manager, const gchar *path, gint val)
{
    GtkAction *action;

    action = gtk_ui_manager_get_action(ui_manager, path);
    if(!action){
        fprintf (stderr, "Warning: set_menu_sensitivity couldn't find action path= %s\n",
                path);
        return;
    }
    gtk_action_set_sensitive (action, val); /* TRUE to make the action sensitive */
}

static void
set_menu_object_data_meat(GtkUIManager *ui_manager, const gchar *path, const gchar *key, gpointer data)
{
    GtkWidget *menu = NULL;

    if ((menu =  gtk_ui_manager_get_widget(ui_manager, path)) != NULL){
        g_object_set_data(G_OBJECT(menu), key, data);
    }else{
#if 0
        g_warning("set_menu_object_data_meat: no menu, path: %s",path);
#endif
    }
}

void
set_menu_object_data (const gchar *path, const gchar *key, gpointer data)
{
    if (strncmp (path,"/Menubar",8) == 0){
        set_menu_object_data_meat(ui_manager_main_menubar, path, key, data);
    }else if (strncmp (path,"/PacketListMenuPopup",20) == 0){
        set_menu_object_data_meat(ui_manager_packet_list_menu, path, key, data);
    }else if (strncmp (path,"/TreeViewPopup",14) == 0){
        set_menu_object_data_meat(ui_manager_tree_view_menu, path, key, data);
    }else if (strncmp (path,"/BytesMenuPopup",15) == 0){
        set_menu_object_data_meat(ui_manager_bytes_menu, path, key, data);
    }else if (strncmp (path,"/ProfilesMenuPopup",18) == 0){
        set_menu_object_data_meat(ui_manager_statusbar_profiles_menu, path, key, data);
    }
}


/* Recently used capture files submenu:
 * Submenu containing the recently used capture files.
 * The capture filenames are always kept with the absolute path to be independent
 * of the current path.
 * They are only stored inside the labels of the submenu (no separate list). */

#define MENU_RECENT_FILES_PATH "/Menubar/FileMenu/OpenRecent"
#define MENU_RECENT_FILES_KEY "Recent File Name"

/* Add a file name to the top of the list; if it's already present remove it first */
static GList *
remove_present_file_name(GList *recent_files_list, const gchar *cf_name)
{
    GList *li, *next;
    gchar *widget_cf_name;

    for (li = g_list_first(recent_files_list); li; li = next) {
        widget_cf_name = (gchar *)li->data;
        next = li->next;
        if (
#ifdef _WIN32
            /* do a case insensitive compare on win32 */
            g_ascii_strncasecmp(widget_cf_name, cf_name, 1000) == 0){
#else   /* _WIN32 */
            /* do a case sensitive compare on unix */
            strncmp(widget_cf_name, cf_name, 1000) == 0 ){
#endif
            recent_files_list = g_list_remove(recent_files_list,widget_cf_name);
        }
    }

    return recent_files_list;
}

static void
recent_changed_cb (GtkUIManager *ui_manager,
                   gpointer          user_data _U_)
{
  guint  merge_id;
  GList *action_groups, *l;


  merge_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (ui_manager),
                               "recent-files-merge-id"));

  /* remove the UI */
  gtk_ui_manager_remove_ui (ui_manager, merge_id);

  /* remove the action group; gtk_ui_manager_remove_action_group()
   * should really take the action group's name instead of its
   * pointer.
   */
  action_groups = gtk_ui_manager_get_action_groups (ui_manager);
  for (l = action_groups; l != NULL; l = l->next)
  {
      GtkActionGroup *group = (GtkActionGroup *)l->data;

      if (strcmp (gtk_action_group_get_name (group), "recent-files-group") == 0){
          /* this unrefs the action group and all of its actions */
          gtk_ui_manager_remove_action_group (ui_manager, group);
          break;
      }
  }

  /* generate a new merge id and re-add everything */
  merge_id = gtk_ui_manager_new_merge_id (ui_manager);
  add_recent_items (merge_id, ui_manager);
}

static void
recent_clear_cb(GtkAction *action _U_, gpointer user_data _U_)
{
    GtkWidget *submenu_recent_files;
    GList     *recent_files_list;

    /* Get the list of recent files, free the list and store the empty list with the widget */
    submenu_recent_files = gtk_ui_manager_get_widget(ui_manager_main_menubar, MENU_RECENT_FILES_PATH);
    recent_files_list = (GList *)g_object_get_data(G_OBJECT(submenu_recent_files), "recent-files-list");
    /* Free the name strings ?? */
    g_list_free(recent_files_list);
    recent_files_list = NULL;
    g_object_set_data(G_OBJECT(submenu_recent_files), "recent-files-list", recent_files_list);
    /* Calling recent_changed_cb will rebuild the GUI call add_recent_items which will in turn call
     * main_welcome_reset_recent_capture_files
     */
    recent_changed_cb(ui_manager_main_menubar, NULL);
}

static void
add_recent_items (guint merge_id, GtkUIManager *ui_manager)
{
    GtkActionGroup *action_group;
    GtkAction      *action;
    GtkWidget      *submenu_recent_files;
    GtkWidget      *submenu_recent_file;
    GList          *items, *l;
    gchar          *action_name;
    gchar          *recent_path;
    guint           i;

    /* Reset the recent files list in the welcome screen */
    main_welcome_reset_recent_capture_files();

    action_group = gtk_action_group_new ("recent-files-group");

    submenu_recent_files = gtk_ui_manager_get_widget(ui_manager, MENU_RECENT_FILES_PATH);
    if(!submenu_recent_files){
        g_warning("add_recent_items: No submenu_recent_files found, path= MENU_RECENT_FILES_PATH");
    }
    items = (GList *)g_object_get_data(G_OBJECT(submenu_recent_files), "recent-files-list");

    gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
    g_object_set_data (G_OBJECT (ui_manager),
                     "recent-files-merge-id", GUINT_TO_POINTER (merge_id));

    /* no items */
    if (!items){

      action = (GtkAction *)g_object_new (GTK_TYPE_ACTION,
                 "name", "recent-info-empty",
                 "label", "No recently used files",
                 "sensitive", FALSE,
                 NULL);
      gtk_action_group_add_action (action_group, action);
      gtk_action_set_sensitive(action, FALSE);
      g_object_unref (action);

      gtk_ui_manager_add_ui (ui_manager, merge_id,
                 "/Menubar/FileMenu/OpenRecent/RecentFiles",
                 "recent-info-empty",
                 "recent-info-empty",
                 GTK_UI_MANAGER_MENUITEM,
                 FALSE);

      return;
    }

  for (i = 0, l = items;
       i < prefs.gui_recent_files_count_max && l != NULL;
       i +=1, l = l->next)
    {
      gchar *item_name = (gchar *)l->data;
      action_name = g_strdup_printf ("recent-info-%u", i);

      action = (GtkAction *)g_object_new (GTK_TYPE_ACTION,
                 "name", action_name,
                 "label", item_name,
                 "stock_id", WIRESHARK_STOCK_SAVE,
                 NULL);
      g_signal_connect (action, "activate",
                        G_CALLBACK (menu_open_recent_file_cmd_cb), NULL);
#if !GTK_CHECK_VERSION(2,16,0)
      g_object_set_data (G_OBJECT (action), "FileName", item_name);
#endif
      gtk_action_group_add_action (action_group, action);
      g_object_unref (action);

      gtk_ui_manager_add_ui (ui_manager, merge_id,
                 "/Menubar/FileMenu/OpenRecent/RecentFiles",
                 action_name,
                 action_name,
                 GTK_UI_MANAGER_MENUITEM,
                 FALSE);

      /* Disable mnemonic accelerator key for recent file name */
      recent_path = g_strdup_printf ("/Menubar/FileMenu/OpenRecent/RecentFiles/recent-info-%u", i);
      submenu_recent_file = gtk_ui_manager_get_widget(ui_manager, recent_path);
      g_object_set(G_OBJECT (submenu_recent_file), "use-underline", 0, NULL);

      /* Add the file name to the recent files list on the Welcome screen */
      main_welcome_add_recent_capture_file(item_name, G_OBJECT(action));

      g_free (recent_path);
      g_free (action_name);
    }
    /* Add a Separator */
    gtk_ui_manager_add_ui (ui_manager, merge_id,
             "/Menubar/FileMenu/OpenRecent/RecentFiles",
             "separator-recent-info",
             NULL,
             GTK_UI_MANAGER_SEPARATOR,
             FALSE);

    /* Add a clear Icon */
    action = (GtkAction *)g_object_new (GTK_TYPE_ACTION,
             "name", "clear-recent-info",
             "label", "Clear the recent files list",
             "stock_id", GTK_STOCK_CLEAR,
             NULL);

    g_signal_connect (action, "activate",
                        G_CALLBACK (recent_clear_cb), NULL);

    gtk_action_group_add_action (action_group, action);
    g_object_unref (action);

    gtk_ui_manager_add_ui (ui_manager, merge_id,
             "/Menubar/FileMenu/OpenRecent/RecentFiles",
             "clear-recent-info",
             "clear-recent-info",
             GTK_UI_MANAGER_MENUITEM,
             FALSE);

}

#define MENU_STATISTICS_PATH "/Menubar/StatisticsMenu"

static void
add_tap_plugins (guint merge_id, GtkUIManager *ui_manager)
{
    GtkActionGroup *action_group;
    GtkAction      *action;
    GtkWidget      *submenu_statistics;
    GList          *cfg_list;
    GList          *iter;
    gchar          *action_name;

    gchar          *submenu_path;
    gsize           submenu_path_size;
    gchar          *stat_name_buf;
    gchar          *stat_name;
    gchar          *sep;

    action_group = gtk_action_group_new ("tap-plugins-group");

    submenu_statistics = gtk_ui_manager_get_widget(ui_manager_main_menubar, MENU_STATISTICS_PATH);
    if(!submenu_statistics){
        g_warning("add_tap_plugins: No submenu_statistics found, path= MENU_STATISTICS_PATH");
        return;
    }
    gtk_ui_manager_insert_action_group (ui_manager_main_menubar, action_group, 0);
    g_object_set_data (G_OBJECT (ui_manager_main_menubar),
                     "tap-plugins-merge-id", GUINT_TO_POINTER (merge_id));

    cfg_list = stats_tree_get_cfg_list();
    iter = g_list_first(cfg_list);
    while (iter) {
        stats_tree_cfg *cfg = (stats_tree_cfg*)iter->data;
        if (cfg->plugin) {
            stat_name_buf = g_strdup(cfg->name);
            submenu_path_size = (gsize)(strlen(MENU_STATISTICS_PATH)+strlen(cfg->name)+strlen(cfg->abbr)+1);   /* worst case length */
            submenu_path = (gchar *)g_malloc(submenu_path_size);
            g_strlcpy(submenu_path, MENU_STATISTICS_PATH, submenu_path_size);

            sep = stat_name= stat_name_buf;
            while ((sep = strchr(sep,'/')) != NULL) {
                if (*(++sep)=='/') {  /* escapeded slash - two slash characters after each other */
                    memmove(sep,sep+1,strlen(sep));
                }
                else {
                    /* we got a new submenu name - ignore the edge case where there is no text following this slash */
                    *(sep-1)= 0;
                    action_name = g_strdup_printf("%s/%s", submenu_path,stat_name);
                    if (!gtk_ui_manager_get_widget(ui_manager, action_name)) {
                        action = (GtkAction *)g_object_new (GTK_TYPE_ACTION,
                            "name", action_name,
                            "label", stat_name,
                            NULL);
                        gtk_action_group_add_action (action_group, action);
                        g_object_unref (action);

                        gtk_ui_manager_add_ui (ui_manager, merge_id,
                            submenu_path,
                            stat_name,
                            action_name,
                            GTK_UI_MANAGER_MENU,
                            FALSE);
                    }
                    g_free (action_name);

                    g_strlcat(submenu_path,"/",submenu_path_size);
                    g_strlcat(submenu_path,stat_name,submenu_path_size);
                    stat_name= sep;
                }
            }

            action_name = g_strdup_printf("%s/%s", submenu_path, cfg->abbr);
            action = (GtkAction *)g_object_new (GTK_TYPE_ACTION,
                 "name", action_name,
                 "label", stat_name,
                 NULL);
            g_signal_connect (action, "activate", G_CALLBACK (gtk_stats_tree_cb), NULL);
            gtk_action_group_add_action (action_group, action);
            g_object_unref (action);

            gtk_ui_manager_add_ui (ui_manager, merge_id,
                 submenu_path,
                 action_name,
                 action_name,
                 GTK_UI_MANAGER_MENUITEM,
                FALSE);

            g_free (action_name);
            g_free (stat_name_buf);
            g_free (submenu_path);
        }
        iter = g_list_next(iter);
    }
    g_list_free(cfg_list);
}

/* Open a file by its name
   (Beware: will not ask to close existing capture file!) */
void
menu_open_filename(gchar *cf_name)
{
    GtkWidget *submenu_recent_files;
    int        err;
    GList     *recent_files_list;


    submenu_recent_files = gtk_ui_manager_get_widget(ui_manager_main_menubar, MENU_RECENT_FILES_PATH);
    if(!submenu_recent_files){
        g_warning("menu_open_filename: No submenu_recent_files found, path= MENU_RECENT_FILES_PATH");
    }
    recent_files_list = (GList *)g_object_get_data(G_OBJECT(submenu_recent_files), "recent-files-list");
    /* XXX: ask user to remove item, it's maybe only a temporary problem */
    /* open and read the capture file (this will close an existing file) */
    if (cf_open(&cfile, cf_name, WTAP_TYPE_AUTO, FALSE, &err) == CF_OK) {
        cf_read(&cfile, FALSE);
    }else{
        recent_files_list = remove_present_file_name(recent_files_list, cf_name);
        g_object_set_data(G_OBJECT(submenu_recent_files), "recent-files-list", recent_files_list);
        /* Calling recent_changed_cb will rebuild the GUI call add_recent_items which will in turn call
         * main_welcome_reset_recent_capture_files
         */
        recent_changed_cb(ui_manager_main_menubar, NULL);
    }
}

/* callback, if the user pushed a recent file submenu item */
static void
menu_open_recent_file_cmd(GtkAction *action)
{
    GtkWidget   *submenu_recent_files;
    GList       *recent_files_list;
    const gchar *cf_name;
    int          err;

#if GTK_CHECK_VERSION(2,16,0)
    cf_name = gtk_action_get_label(action);
#else
    cf_name = (const gchar *)g_object_get_data(G_OBJECT(action), "FileName");
#endif

    /* open and read the capture file (this will close an existing file) */
    if (cf_open(&cfile, cf_name, WTAP_TYPE_AUTO, FALSE, &err) == CF_OK) {
        cf_read(&cfile, FALSE);
    } else {
        submenu_recent_files = gtk_ui_manager_get_widget(ui_manager_main_menubar, MENU_RECENT_FILES_PATH);
        recent_files_list = (GList *)g_object_get_data(G_OBJECT(submenu_recent_files), "recent-files-list");

        recent_files_list = remove_present_file_name(recent_files_list, cf_name);
        g_object_set_data(G_OBJECT(submenu_recent_files), "recent-files-list", recent_files_list);
        /* Calling recent_changed_cb will rebuild the GUI call add_recent_items which will in turn call
         * main_welcome_reset_recent_capture_files
         */
        recent_changed_cb(ui_manager_main_menubar, NULL);
    }
}

static void
menu_open_recent_file_cmd_cb(GtkAction *action, gpointer data _U_)
{
    /* If there's unsaved data, let the user save it first.
       If they cancel out of it, don't open the file. */
    if (do_file_close(&cfile, FALSE, " before opening a new capture file"))
        menu_open_recent_file_cmd(action);
}

static void
add_menu_recent_capture_file_absolute(const gchar *cf_name)
{
    GtkWidget *submenu_recent_files;
    GList     *li;
    gchar     *widget_cf_name;
    gchar     *normalized_cf_name;
    guint      cnt;
    GList     *recent_files_list;

    normalized_cf_name = g_strdup(cf_name);
#ifdef _WIN32
    /* replace all slashes by backslashes */
    g_strdelimit(normalized_cf_name, "/", '\\');
#endif

    /* get the submenu container item */
    submenu_recent_files = gtk_ui_manager_get_widget(ui_manager_main_menubar, MENU_RECENT_FILES_PATH);
    if(!submenu_recent_files){
        g_warning("add_menu_recent_capture_file_absolute: No submenu_recent_files found, path= MENU_RECENT_FILES_PATH");
        g_free(normalized_cf_name);
        return;
    }
    recent_files_list = (GList *)g_object_get_data(G_OBJECT(submenu_recent_files), "recent-files-list");
    cnt = 1;
    for (li = g_list_first(recent_files_list); li; cnt++) {
        widget_cf_name = (gchar *)li->data;

        /* Find the next element BEFORE we (possibly) free the current one below */
        li = li->next;

        if (
#ifdef _WIN32
            /* do a case insensitive compare on win32 */
            g_ascii_strncasecmp(widget_cf_name, normalized_cf_name, 1000) == 0 ||
#else   /* _WIN32 */
            /* do a case sensitive compare on unix */
            strncmp(widget_cf_name, normalized_cf_name, 1000) == 0 ||
#endif
            cnt >= prefs.gui_recent_files_count_max) {
            recent_files_list = g_list_remove(recent_files_list,widget_cf_name);
            cnt--;
        }
    }
    recent_files_list = g_list_prepend(recent_files_list, normalized_cf_name);
    g_object_set_data(G_OBJECT(submenu_recent_files), "recent-files-list", recent_files_list);
    recent_changed_cb( ui_manager_main_menubar, NULL);
}


/* add the capture filename to the "Recent Files" menu */
/* (will change nothing, if this filename is already in the menu) */
/*
 * XXX - We might want to call SHAddToRecentDocs under Windows 7:
 * http:                        //stackoverflow.com/questions/437212/how-do-you-register-a-most-recently-used-list-with-windows-in-preparation-for-win
 */
void
add_menu_recent_capture_file(const gchar *cf_name)
{
    gchar *curr;
    gchar *absolute;

    /* if this filename is an absolute path, we can use it directly */
    if (g_path_is_absolute(cf_name)) {
        add_menu_recent_capture_file_absolute(cf_name);
        return;
    }

    /* this filename is not an absolute path, prepend the current dir */
    curr = g_get_current_dir();
    absolute = g_strdup_printf("%s%s%s", curr, G_DIR_SEPARATOR_S, cf_name);
    add_menu_recent_capture_file_absolute(absolute);
    g_free(curr);
    g_free(absolute);
}


/* write all capture filenames of the menu to the user's recent file */
void
menu_recent_file_write_all(FILE *rf)
{
    GtkWidget *submenu_recent_files;
    gchar     *cf_name;
    GList     *recent_files_list, *list;

    submenu_recent_files = gtk_ui_manager_get_widget(ui_manager_main_menubar, MENU_RECENT_FILES_PATH);
    if(!submenu_recent_files){
        g_warning("menu_recent_file_write_all: No submenu_recent_files found, path= MENU_RECENT_FILES_PATH");
    }
    recent_files_list = (GList *)g_object_get_data(G_OBJECT(submenu_recent_files), "recent-files-list");
    list =  g_list_last(recent_files_list);
    while (list != NULL) {
        cf_name = (gchar *)list->data;
        if (cf_name) {
            fprintf (rf, RECENT_KEY_CAPTURE_FILE ": %s\n", cf_name);
        }
        list = g_list_previous(list);
    }
    g_list_free(recent_files_list);
}

void
menu_name_resolution_changed(void)
{
    GtkWidget *menu;

    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/NameResolution/EnableforMACLayer");
    if(!menu){
        g_warning("menu_name_resolution_changed: No menu found, path= /Menubar/ViewMenu/NameResolution/EnableforMACLayer");
    }
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), gbl_resolv_flags.mac_name);

    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/NameResolution/EnableforNetworkLayer");
    if(!menu){
        g_warning("menu_name_resolution_changed: No menu found, path= /Menubar/ViewMenu/NameResolution/EnableforNetworkLayer");
    }
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), gbl_resolv_flags.network_name);

    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/NameResolution/EnableforTransportLayer");
    if(!menu){
        g_warning("menu_name_resolution_changed: No menu found, path= /Menubar/ViewMenu/NameResolution/EnableforTransportLayer");
    }
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), gbl_resolv_flags.transport_name);

    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/NameResolution/UseExternalNetworkNameResolver");
    if(!menu){
        g_warning("menu_name_resolution_changed: No menu found, path= /Menubar/ViewMenu/NameResolution/UseExternalNetworkNameResolver");
    }
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), gbl_resolv_flags.use_external_net_name_resolver);
}

static void
menu_name_resolution_update_cb(GtkAction *action _U_, gpointer data _U_)
{
    menu_name_resolution_changed();
}

static void
name_resolution_cb(GtkWidget *w, gpointer d _U_, gboolean* res_flag)
{
    if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w))) {
        *res_flag = TRUE;
    } else {
        *res_flag = FALSE;
    }

    packet_list_recreate();
    redraw_packet_bytes_all();
}

#ifdef HAVE_LIBPCAP
void
menu_auto_scroll_live_changed(gboolean auto_scroll_live_in)
{
    GtkWidget *menu;

    /* tell menu about it */
    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/AutoScrollinLiveCapture");
    if(!menu){
        g_warning("menu_auto_scroll_live_changed: No menu found, path= /Menubar/ViewMenu/AutoScrollinLiveCapture");
    }
    if( ((gboolean) gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu)) != auto_scroll_live_in) ) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), auto_scroll_live_in);
    }
}
#endif /*HAVE_LIBPCAP */




void
menu_colorize_changed(gboolean packet_list_colorize)
{
    GtkWidget *menu;

    /* tell menu about it */
    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/ColorizePacketList");
    if(!menu){
        g_warning("menu_colorize_changed: No menu found, path= /Menubar/ViewMenu/ColorizePacketList");
    }
    if( (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu)) != packet_list_colorize) ) {
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), packet_list_colorize);
    }
}

static void
colorize_cb(GtkWidget *w, gpointer d _U_)
{
    main_colorize_changed(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(w)));
}


/* the recent file read has finished, update the menu corresponding */
void
menu_recent_read_finished(void)
{
    GtkWidget *menu;

    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/MainToolbar");
    if(!menu){
        g_warning("menu_recent_read_finished: No menu found, path= /Menubar/ViewMenu/MainToolbar");
    }else{
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), recent.main_toolbar_show);
    }
    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/FilterToolbar");
    if(!menu){
        g_warning("menu_recent_read_finished: No menu found, path= /Menubar/ViewMenu/FilterToolbar");
    }else{
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), recent.filter_toolbar_show);
    };
    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/WirelessToolbar");
    if(!menu){
        g_warning("menu_recent_read_finished: No menu found, path= /Menubar/ViewMenu/WirelessToolbar");
    }else{
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), recent.wireless_toolbar_show);
    }

    /* Fix me? */
    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/StatusBar");
    if(!menu){
        g_warning("menu_recent_read_finished: No menu found, path= /Menubar/ViewMenu/StatusBar");
    }else{
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), recent.statusbar_show);
    }

    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/PacketList");
    if(!menu){
        g_warning("menu_recent_read_finished: No menu found, path= /Menubar/ViewMenu/PacketList");
    }else{
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), recent.packet_list_show);
    }

    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/PacketDetails");
    if(!menu){
        g_warning("menu_recent_read_finished: No menu found, path= /Menubar/ViewMenu/PacketDetails");
    }else{
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), recent.tree_view_show);
    }

    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/PacketBytes");
    if(!menu){
        g_warning("menu_recent_read_finished: No menu found, path= /Menubar/ViewMenu/PacketBytes");
    }else{
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), recent.byte_view_show);
    }

    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/ColorizePacketList");
    if(!menu){
        g_warning("menu_recent_read_finished: No menu found, path= /Menubar/ViewMenu/ColorizePacketList");
    }else{
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), recent.packet_list_colorize);
    }

    menu_name_resolution_changed();

#ifdef HAVE_LIBPCAP
    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/AutoScrollinLiveCapture");
    if(!menu){
        g_warning("menu_recent_read_finished: No menu found, path= /Menubar/ViewMenu/AutoScrollinLiveCapture");
    }else{
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), auto_scroll_live);
    }
#endif
    main_widgets_rearrange();

    /* Update the time format if we had a command line value. */
    if (global_commandline_info.time_format != TS_NOT_SET) {
        recent.gui_time_format = global_commandline_info.time_format;
    }

    /* XXX Fix me */
    timestamp_set_type(recent.gui_time_format);
    /* This call adjusts column width */
    cf_timestamp_auto_precision(&cfile);
    packet_list_queue_draw();
    /* the actual precision will be set in packet_list_queue_draw() below */
    if (recent.gui_time_precision > TS_PREC_FIXED_NSEC) {
        timestamp_set_precision(TS_PREC_AUTO);
    } else {
        timestamp_set_precision(recent.gui_time_precision);
    }
    /* This call adjusts column width */
    cf_timestamp_auto_precision(&cfile);
    packet_list_queue_draw();

    /* don't change the seconds format, if we had a command line value */
    if (timestamp_get_seconds_type() != TS_SECONDS_NOT_SET) {
        recent.gui_seconds_format = timestamp_get_seconds_type();
    }
    menu = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/TimeDisplayFormat/DisplaySecondsWithHoursAndMinutes");
    if(!menu){
        g_warning("menu_recent_read_finished: No menu found, path= /Menubar/ViewMenu/TimeDisplayFormat/DisplaySecondsWithHoursAndMinutes");
    }

    switch (recent.gui_seconds_format) {
    case TS_SECONDS_DEFAULT:
        recent.gui_seconds_format = (ts_seconds_type)-1;
        /* set_active will not trigger the callback when deactivating an inactive item! */
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), TRUE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), FALSE);
        break;
    case TS_SECONDS_HOUR_MIN_SEC:
        recent.gui_seconds_format = (ts_seconds_type)-1;
        /* set_active will not trigger the callback when activating an active item! */
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), FALSE);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), TRUE);
        break;
    default:
        g_assert_not_reached();
    }

    main_colorize_changed(recent.packet_list_colorize);
}


gboolean
popup_menu_handler(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    GtkWidget      *menu         = (GtkWidget *)data;
    GdkEventButton *event_button = NULL;
    gint            row, column;

    if(widget == NULL || event == NULL || data == NULL) {
        return FALSE;
    }

    /*
     * If we ever want to make the menu differ based on what row
     * and/or column we're above, we'd use "eth_clist_get_selection_info()"
     * to find the row and column number for the coordinates; a CTree is,
     * I guess, like a CList with one column(?) and the expander widget
     * as a pixmap.
     */
    /* Check if we are on packet_list object */
    if (widget == g_object_get_data(G_OBJECT(popup_menu_object), E_MPACKET_LIST_KEY) &&
        ((GdkEventButton *)event)->button != 1) {
        gint physical_row;
        if (packet_list_get_event_row_column((GdkEventButton *)event, &physical_row, &row, &column)) {
            g_object_set_data(G_OBJECT(popup_menu_object), E_MPACKET_LIST_ROW_KEY,
                            GINT_TO_POINTER(row));
            g_object_set_data(G_OBJECT(popup_menu_object), E_MPACKET_LIST_COL_KEY,
                            GINT_TO_POINTER(column));
            packet_list_set_selected_row(row);
        }
    }

    /* Check if we are on tree_view object */
    if (widget == tree_view_gbl) {
        tree_view_select(widget, (GdkEventButton *) event);
    }

    /* context menu handler */
    if(event->type == GDK_BUTTON_PRESS) {
        event_button = (GdkEventButton *) event;

        /* To quote the "Gdk Event Structures" doc:
         * "Normally button 1 is the left mouse button, 2 is the middle button, and 3 is the right button" */
        if(event_button->button == 3) {
            gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                           event_button->button,
                           event_button->time);
            g_signal_stop_emission_by_name(widget, "button_press_event");
            return TRUE;
        }
    }

    /* Check if we are on byte_view object */
    if(widget == get_notebook_bv_ptr(byte_nb_ptr_gbl)) {
        byte_view_select(widget, (GdkEventButton *) event);
    }

    /* GDK_2BUTTON_PRESS is a doubleclick -> expand/collapse tree row */
    /* GTK version 1 seems to be doing this automatically */
    if (widget == tree_view_gbl && event->type == GDK_2BUTTON_PRESS) {
        GtkTreePath      *path;

        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
                                          (gint) (((GdkEventButton *)event)->x),
                                          (gint) (((GdkEventButton *)event)->y),
                                          &path, NULL, NULL, NULL))
        {
            if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(widget), path))
                gtk_tree_view_collapse_row(GTK_TREE_VIEW(widget), path);
            else
                gtk_tree_view_expand_row(GTK_TREE_VIEW(widget), path,
                                         FALSE);
            gtk_tree_path_free(path);
        }
    }
    return FALSE;
}

/* Enable or disable menu items based on whether you have a capture file
   you've finished reading and, if you have one, whether it's been saved
   and whether it could be saved except by copying the raw packet data. */
void
set_menus_for_capture_file(capture_file *cf)
{
    if (cf == NULL || cf->state == FILE_READ_IN_PROGRESS) {
        /* We have no capture file or we're currently reading a file */
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/Merge", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/Close", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/Save", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/SaveAs", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportSpecifiedPackets", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportPacketDissections", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportSelectedPacketBytes", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportSSLSessionKeys", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportObjects", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportPDUs", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/Reload", FALSE);
    } else {
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/Merge", cf_can_write_with_wiretap(cf));
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/Close", TRUE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/Save",
                             cf_can_save(cf));
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/SaveAs",
                             cf_can_save_as(cf));
        /*
         * "Export Specified Packets..." should be available only if
         * we can write the file out in at least one format.
         */
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportSpecifiedPackets",
                             cf_can_write_with_wiretap(cf));
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportPacketDissections", TRUE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportSelectedPacketBytes", TRUE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportSSLSessionKeys", TRUE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportObjects", TRUE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportPDUs", TRUE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/Reload", TRUE);
    }
}

/* Enable or disable menu items based on whether there's a capture in
   progress. */
void
set_menus_for_capture_in_progress(gboolean capture_in_progress)
{
    /* Either a capture was started or stopped; in either case, it's not
       in the process of stopping, so allow quitting. */
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/Quit",
                         TRUE);

    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/Open",
                         !capture_in_progress);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/OpenRecent",
                         !capture_in_progress);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportPacketDissections",
                         capture_in_progress);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportSelectedPacketBytes",
                         capture_in_progress);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportSSLSessionKeys",
                         capture_in_progress);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/ExportObjects",
                         capture_in_progress);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/Set",
                         !capture_in_progress);
    set_menu_sensitivity(ui_manager_packet_list_heading, "/PacketListHeadingPopup/SortAscending",
                         !capture_in_progress);
    set_menu_sensitivity(ui_manager_packet_list_heading, "/PacketListHeadingPopup/SortDescending",
                         !capture_in_progress);
    set_menu_sensitivity(ui_manager_packet_list_heading, "/PacketListHeadingPopup/NoSorting",
                         !capture_in_progress);

#ifdef HAVE_LIBPCAP
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/CaptureMenu/Options",
                         !capture_in_progress);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/CaptureMenu/Start",
                         !capture_in_progress && global_capture_opts.num_selected > 0);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/CaptureMenu/Stop",
                         capture_in_progress);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/CaptureMenu/Restart",
                         capture_in_progress);
#endif /* HAVE_LIBPCAP */
}

#ifdef HAVE_LIBPCAP
void
set_menus_capture_start_sensitivity(gboolean enable)
{
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/CaptureMenu/Start",
                         enable);
}
#endif

/* Disable menu items while we're waiting for the capture child to
   finish.  We disallow quitting until it finishes, and also disallow
   stopping or restarting the capture. */
void
set_menus_for_capture_stopping(void)
{
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/Quit",
                         FALSE);
#ifdef HAVE_LIBPCAP
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/CaptureMenu/Stop",
                         FALSE);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/CaptureMenu/Restart",
                         FALSE);
#endif /* HAVE_LIBPCAP */
}


void
set_menus_for_captured_packets(gboolean have_captured_packets)
{
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/Print",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/Print",
                         have_captured_packets);

    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/FindPacket",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/FindNext",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/FindPrevious",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/ZoomIn",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/ZoomOut",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/NormalSize",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/GoMenu/Goto",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/GoMenu/PreviousPacket",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/GoMenu/NextPacket",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/GoMenu/FirstPacket",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/GoMenu/LastPacket",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/GoMenu/PreviousPacketInConversation",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/GoMenu/NextPacketInConversation",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/StatisticsMenu/Summary",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/StatisticsMenu/ShowCommentsSummary",
                         have_captured_packets);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/StatisticsMenu/ProtocolHierarchy",
                         have_captured_packets);
}

void
set_menus_for_selected_packet(capture_file *cf)
{
    GList      *conv_filter_list_entry;
    guint       i          = 0;
    gboolean    properties = FALSE;
    const char *abbrev     = NULL;
    char       *prev_abbrev;
    gboolean is_ip = FALSE, is_tcp = FALSE, is_udp = FALSE, is_sctp = FALSE, is_ssl = FALSE, is_lte_rlc = FALSE, is_http = FALSE;

    /* Making the menu context-sensitive allows for easier selection of the
       desired item and has the added benefit, with large captures, of
       avoiding needless looping through huge lists for marked, ignored,
       or time-referenced packets. */
    gboolean frame_selected = cf->current_frame != NULL;
        /* A frame is selected */
    gboolean have_marked = frame_selected && cf->marked_count > 0;
        /* We have marked frames.  (XXX - why check frame_selected?) */
    gboolean another_is_marked = have_marked &&
        !(cf->marked_count == 1 && cf->current_frame->flags.marked);
        /* We have a marked frame other than the current frame (i.e.,
           we have at least one marked frame, and either there's more
           than one marked frame or the current frame isn't marked). */
    gboolean have_time_ref = cf->ref_time_count > 0;
    gboolean another_is_time_ref = frame_selected && have_time_ref &&
        !(cf->ref_time_count == 1 && cf->current_frame->flags.ref_time);
        /* We have a time reference frame other than the current frame (i.e.,
           we have at least one time reference frame, and either there's more
           than one time reference frame or the current frame isn't a
           time reference frame). (XXX - why check frame_selected?) */
    if (cf->edt)
    {
        proto_get_frame_protocols(cf->edt->pi.layers, &is_ip, &is_tcp, &is_udp, &is_sctp, &is_ssl, NULL, &is_lte_rlc);
        is_http = proto_is_frame_protocol(cf->edt->pi.layers, "http");
    }
    if (cf->edt && cf->edt->tree) {
        GPtrArray          *ga;
        header_field_info  *hfinfo;
        field_info         *v;
        guint              ii;

        ga = proto_all_finfos(cf->edt->tree);

        for (ii = ga->len - 1; ii > 0 ; ii -= 1) {

            v = (field_info *)g_ptr_array_index (ga, ii);
            hfinfo =  v->hfinfo;

            if (!g_str_has_prefix(hfinfo->abbrev, "text") &&
                !g_str_has_prefix(hfinfo->abbrev, "_ws.expert") &&
                !g_str_has_prefix(hfinfo->abbrev, "_ws.malformed")) {

                if (hfinfo->parent == -1) {
                    abbrev = hfinfo->abbrev;
                } else {
                    abbrev = proto_registrar_get_abbrev(hfinfo->parent);
                }
                properties = prefs_is_registered_protocol(abbrev);
                break;
            }
        }
    }

    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/MarkPacket",
                         frame_selected);
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/MarkPacket",
                         frame_selected);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/MarkAllDisplayedPackets",
                         cf->displayed_count > 0);
    /* Unlike un-ignore, do not allow unmark of all frames when no frames are displayed  */
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/UnmarkAllDisplayedPackets",
                         have_marked);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/FindNextMark",
                         another_is_marked);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/FindPreviousMark",
                         another_is_marked);

    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/IgnorePacket",
                         frame_selected);
#ifdef WANT_PACKET_EDITOR
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/EditPacket",
                         prefs.gui_packet_editor ? frame_selected : FALSE);
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/EditPacket",
                         prefs.gui_packet_editor ? frame_selected : FALSE);
#endif /* WANT_PACKET_EDITOR */
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/AddEditPktComment",
                         frame_selected && wtap_dump_can_write(cf->linktypes, WTAP_COMMENT_PER_PACKET));
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/AddEditCaptureComment",
                         frame_selected && wtap_dump_can_write(cf->linktypes, WTAP_COMMENT_PER_PACKET));
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/IgnorePacket",
                         frame_selected);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/IgnoreAllDisplayedPackets",
                         cf->displayed_count > 0 && cf->displayed_count != cf->count);
    /* Allow un-ignore of all frames even with no frames currently displayed */
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/Un-IgnoreAllPackets",
                         cf->ignored_count > 0);

    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/SetTimeReference",
                         frame_selected);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/Un-TimeReferenceAllPackets",
                         have_time_ref);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/TimeShift",
                         cf->count > 0);
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/SetTimeReference",
                         frame_selected);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/FindNextTimeReference",
                         another_is_time_ref);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/FindPreviousTimeReference",
                         another_is_time_ref);

    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/ResizeAllColumns",
                         frame_selected);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/CollapseAll",
                         frame_selected);
    set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/CollapseAll",
                         frame_selected);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/ExpandAll",
                         frame_selected);
    set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/ExpandAll",
                         frame_selected);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/ColorizeConversation",
                         frame_selected);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/ResetColoring1-10",
                         tmp_color_filters_used());
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/ShowPacketinNewWindow",
                         frame_selected);
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/ShowPacketinNewWindow",
                         frame_selected);
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/ManuallyResolveAddress",
                         frame_selected ? is_ip : FALSE);
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/SCTP",
                         frame_selected ? is_sctp : FALSE);
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/FollowTCPStream",
                         is_tcp);
    set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/FollowTCPStream",
                         is_tcp);
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/FollowUDPStream",
                         frame_selected ? is_udp : FALSE);
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/FollowSSLStream",
                         frame_selected ? is_ssl : FALSE);
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/FollowHTTPStream",
                         frame_selected ? is_http : FALSE);

    set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/FollowSSLStream",
                         frame_selected ? is_ssl : FALSE);
    set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/FollowUDPStream",
                         frame_selected ? is_udp : FALSE);
    set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/FollowHTTPStream",
                         frame_selected ? is_http : FALSE);

    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/ColorizeConversation",
                         frame_selected);
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/DecodeAs",
                         frame_selected && decode_as_ok());

    if (properties) {
        prev_abbrev = (char *)g_object_get_data(G_OBJECT(ui_manager_packet_list_menu), "menu_abbrev");
        if (!prev_abbrev || (strcmp(prev_abbrev, abbrev) != 0)) {
          /*No previous protocol or protocol changed - update Protocol Preferences menu*/
            module_t *prefs_module_p = prefs_find_module(abbrev);
            rebuild_protocol_prefs_menu(prefs_module_p, properties, ui_manager_packet_list_menu, "/PacketListMenuPopup/ProtocolPreferences");

            g_object_set_data(G_OBJECT(ui_manager_packet_list_menu), "menu_abbrev", g_strdup(abbrev));
            g_free (prev_abbrev);
        }
    }

    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/ProtocolPreferences",
                             properties);
    set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/DecodeAs",
                         frame_selected && decode_as_ok());
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/Copy",
                         frame_selected);
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/ApplyAsFilter",
                         frame_selected);
    set_menu_sensitivity(ui_manager_packet_list_menu, "/PacketListMenuPopup/PrepareaFilter",
                         frame_selected);
    set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/ResolveName",
                         frame_selected && (gbl_resolv_flags.mac_name || gbl_resolv_flags.network_name ||
                                            gbl_resolv_flags.transport_name));
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/AnalyzeMenu/FollowTCPStream",
                         is_tcp);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/AnalyzeMenu/FollowUDPStream",
                         frame_selected ? is_udp : FALSE);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/AnalyzeMenu/FollowSSLStream",
                         frame_selected ? is_ssl : FALSE);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/AnalyzeMenu/FollowHTTPStream",
                         frame_selected ? is_http : FALSE);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/AnalyzeMenu/DecodeAs",
                         frame_selected && decode_as_ok());
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/NameResolution/ResolveName",
                         frame_selected && (gbl_resolv_flags.mac_name || gbl_resolv_flags.network_name ||
                                            gbl_resolv_flags.transport_name));
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ToolsMenu/FirewallACLRules",
                         frame_selected);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/StatisticsMenu/TCPStreamGraphMenu",
                         is_tcp);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/TelephonyMenu/LTEmenu/LTE_RLC_Graph",
                         is_lte_rlc);

    for (i = 0, conv_filter_list_entry = conv_filter_list;
         conv_filter_list_entry != NULL;
         conv_filter_list_entry = g_list_next(conv_filter_list_entry), i++) {
        conversation_filter_t *filter_entry;
        gchar *path;

        filter_entry = (conversation_filter_t *)conv_filter_list_entry->data;
        path = g_strdup_printf("/Menubar/AnalyzeMenu/ConversationFilterMenu/Filters/filter-%u", i);
        set_menu_sensitivity(ui_manager_main_menubar, path,
            menu_dissector_filter_spe_cb(/* frame_data *fd _U_*/ NULL, cf->edt, filter_entry));
        g_free(path);

        path = g_strdup_printf("/PacketListMenuPopup/ConversationFilter/Conversations/color_conversation-%d", i);
        set_menu_sensitivity(ui_manager_packet_list_menu, path,
            menu_color_dissector_filter_spe_cb(NULL, cf->edt, filter_entry));
        g_free(path);

        path = g_strdup_printf("/PacketListMenuPopup/ColorizeConversation/Colorize/%s", filter_entry->display_name);
        set_menu_sensitivity(ui_manager_packet_list_menu, path,
            menu_color_dissector_filter_spe_cb(NULL, cf->edt, filter_entry));
        g_free(path);
    }
}


static void
menu_prefs_toggle_bool (GtkWidget *w, gpointer data)
{
    gboolean *value  = (gboolean *)data;
    module_t *module = (module_t *)g_object_get_data (G_OBJECT(w), "module");

    module->prefs_changed = TRUE;
    *value = !(*value);

    prefs_apply (module);
    if (!prefs.gui_use_pref_save) {
        prefs_main_write();
    }
    redissect_packets();
    redissect_all_packet_windows();
}

static void
menu_prefs_change_enum (GtkWidget *w, gpointer data)
{
    gint     *value     = (gint *)data;
    module_t *module    = (module_t *)g_object_get_data (G_OBJECT(w), "module");
    gint      new_value = GPOINTER_TO_INT(g_object_get_data (G_OBJECT(w), "enumval"));

    if (!gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(w)))
        return;

    if (*value != new_value) {
        module->prefs_changed = TRUE;
        *value = new_value;

        prefs_apply (module);
        if (!prefs.gui_use_pref_save) {
            prefs_main_write();
        }
        redissect_packets();
        redissect_all_packet_windows();
    }
}

void
menu_prefs_reset(void)
{
    g_free (g_object_get_data(G_OBJECT(ui_manager_tree_view_menu), "menu_abbrev"));
    g_object_set_data(G_OBJECT(ui_manager_tree_view_menu), "menu_abbrev", NULL);
}

static void
menu_prefs_change_ok (GtkWidget *w, gpointer parent_w)
{
    GtkWidget   *entry     = (GtkWidget *)g_object_get_data (G_OBJECT(w), "entry");
    module_t    *module    = (module_t *)g_object_get_data (G_OBJECT(w), "module");
    pref_t      *pref      = (pref_t *)g_object_get_data (G_OBJECT(w), "pref");
    const gchar *new_value = gtk_entry_get_text(GTK_ENTRY(entry));
    gchar       *p;
    guint        uval;

    switch (pref->type) {
    case PREF_UINT:
        uval = (guint)strtoul(new_value, &p, pref->info.base);
        if (p == new_value || *p != '\0') {
            simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
                          "The value \"%s\" isn't a valid number.",
                          new_value);
            return;
        }
        if (*pref->varp.uint != uval) {
            module->prefs_changed = TRUE;
            *pref->varp.uint = uval;
        }
        break;
    case PREF_STRING:
        prefs_set_string_like_value(pref, new_value, &module->prefs_changed);
        break;
    case PREF_RANGE:
        if (!prefs_set_range_value(pref, new_value, &module->prefs_changed)) {
            simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
                          "The value \"%s\" isn't a valid range.",
                          new_value);
            return;
        }
        break;
    default:
        g_assert_not_reached();
        break;
    }

    if (module->prefs_changed) {
        /* Ensure we reload the sub menu */
        menu_prefs_reset();
        prefs_apply (module);
        if (!prefs.gui_use_pref_save) {
            prefs_main_write();
        }
        redissect_packets();
        redissect_all_packet_windows();
    }

    window_destroy(GTK_WIDGET(parent_w));
}

static void
menu_prefs_change_cancel (GtkWidget *w _U_, gpointer parent_w)
{
    window_destroy(GTK_WIDGET(parent_w));
}

static void
menu_prefs_edit_dlg (GtkWidget *w, gpointer data)
{
    pref_t    *pref   = (pref_t *)data;
    module_t  *module = (module_t *)g_object_get_data (G_OBJECT(w), "module");
    gchar     *value  = NULL, *tmp_value, *label_str;

    GtkWidget *win, *main_grid, *main_vb, *bbox, *cancel_bt, *ok_bt;
    GtkWidget *entry, *label;

    switch (pref->type) {
    case PREF_UINT:
        switch (pref->info.base) {
        case 8:
            value = g_strdup_printf("%o", *pref->varp.uint);
            break;
        case 10:
            value = g_strdup_printf("%u", *pref->varp.uint);
            break;
        case 16:
            value = g_strdup_printf("%x", *pref->varp.uint);
            break;
        default:
            g_assert_not_reached();
            break;
        }
        break;
    case PREF_STRING:
        value = g_strdup(*pref->varp.string);
        break;
    case PREF_RANGE:
        /* Convert wmem to g_alloc memory */
        tmp_value = range_convert_range(NULL, *pref->varp.range);
        value = g_strdup(tmp_value);
        wmem_free(NULL, tmp_value);
        break;
    default:
        g_assert_not_reached();
        break;
    }

    win = dlg_window_new(module->description);

    gtk_window_set_resizable(GTK_WINDOW(win),FALSE);
    gtk_window_resize(GTK_WINDOW(win), 400, 100);

    main_vb = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 5, FALSE);
    gtk_container_add(GTK_CONTAINER(win), main_vb);
    gtk_container_set_border_width(GTK_CONTAINER(main_vb), 6);

    main_grid = ws_gtk_grid_new();
    gtk_box_pack_start(GTK_BOX(main_vb), main_grid, FALSE, FALSE, 0);
    ws_gtk_grid_set_column_spacing(GTK_GRID(main_grid), 10);

    label_str = g_strdup_printf("%s:", pref->title);
    label = gtk_label_new(label_str);
    g_free(label_str);
    ws_gtk_grid_attach_defaults(GTK_GRID(main_grid), label, 0, 1, 1, 1);
    gtk_misc_set_alignment(GTK_MISC(label), 1.0f, 0.5f);
    if (pref->description)
        gtk_widget_set_tooltip_text(label, pref->description);

    entry = gtk_entry_new();
    ws_gtk_grid_attach_defaults(GTK_GRID(main_grid), entry, 1, 1, 1, 1);
    gtk_entry_set_text(GTK_ENTRY(entry), value);
    if (pref->description)
        gtk_widget_set_tooltip_text(entry, pref->description);

    bbox = dlg_button_row_new(GTK_STOCK_CANCEL,GTK_STOCK_OK, NULL);
    gtk_box_pack_end(GTK_BOX(main_vb), bbox, FALSE, FALSE, 0);

    ok_bt = (GtkWidget *)g_object_get_data(G_OBJECT(bbox), GTK_STOCK_OK);
    g_object_set_data (G_OBJECT(ok_bt), "module", module);
    g_object_set_data (G_OBJECT(ok_bt), "entry", entry);
    g_object_set_data (G_OBJECT(ok_bt), "pref", pref);
    g_signal_connect(ok_bt, "clicked", G_CALLBACK(menu_prefs_change_ok), win);

    dlg_set_activate(entry, ok_bt);

    cancel_bt = (GtkWidget *)g_object_get_data(G_OBJECT(bbox), GTK_STOCK_CANCEL);
    g_signal_connect(cancel_bt, "clicked", G_CALLBACK(menu_prefs_change_cancel), win);
    window_set_cancel_button(win, cancel_bt, NULL);

    gtk_widget_grab_default(ok_bt);
    gtk_widget_show_all(win);
    g_free(value);
}

static guint
add_protocol_prefs_generic_menu(pref_t *pref, gpointer data, GtkUIManager *ui_menu, const char *path)
{
    GtkWidget        *menu_preferences;
    GtkWidget        *menu_item, *menu_sub_item, *sub_menu;
    GSList           *group  = NULL;
    module_t         *module = (module_t *)data;
    const enum_val_t *enum_valp;
    gchar            *label  = NULL, *tmp_str;

    switch (pref->type) {
    case PREF_UINT:
        switch (pref->info.base) {
        case 8:
            label = g_strdup_printf ("%s: %o", pref->title, *pref->varp.uint);
            break;
        case 10:
            label = g_strdup_printf ("%s: %u", pref->title, *pref->varp.uint);
            break;
        case 16:
            label = g_strdup_printf ("%s: %x", pref->title, *pref->varp.uint);
            break;
        default:
            g_assert_not_reached();
            break;
        }
        menu_item = gtk_menu_item_new_with_label(label);
        g_object_set_data (G_OBJECT(menu_item), "module", module);
        g_signal_connect(menu_item, "activate", G_CALLBACK(menu_prefs_edit_dlg), pref);
        g_free (label);
        break;
    case PREF_BOOL:
        menu_item = gtk_check_menu_item_new_with_label(pref->title);
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(menu_item), *pref->varp.boolp);
        g_object_set_data (G_OBJECT(menu_item), "module", module);
        g_signal_connect(menu_item, "activate", G_CALLBACK(menu_prefs_toggle_bool), pref->varp.boolp);
        break;
    case PREF_ENUM:
        menu_item = gtk_menu_item_new_with_label(pref->title);
        sub_menu = gtk_menu_new();
        gtk_menu_item_set_submenu (GTK_MENU_ITEM(menu_item), sub_menu);
        enum_valp = pref->info.enum_info.enumvals;
        while (enum_valp->name != NULL) {
            menu_sub_item = gtk_radio_menu_item_new_with_label(group, enum_valp->description);
            group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menu_sub_item));
            if (enum_valp->value == *pref->varp.enump) {
                gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(menu_sub_item), TRUE);
            }
            g_object_set_data (G_OBJECT(menu_sub_item), "module", module);
            g_object_set_data (G_OBJECT(menu_sub_item), "enumval", GINT_TO_POINTER(enum_valp->value));
            g_signal_connect(menu_sub_item, "activate", G_CALLBACK(menu_prefs_change_enum), pref->varp.enump);
            gtk_menu_shell_append (GTK_MENU_SHELL(sub_menu), menu_sub_item);
            gtk_widget_show (menu_sub_item);
            enum_valp++;
        }
        break;
    case PREF_STRING:
        label = g_strdup_printf ("%s: %s", pref->title, *pref->varp.string);
        menu_item = gtk_menu_item_new_with_label(label);
        g_object_set_data (G_OBJECT(menu_item), "module", module);
        g_signal_connect(menu_item, "activate", G_CALLBACK(menu_prefs_edit_dlg), pref);
        g_free (label);
        break;
    case PREF_RANGE:
        tmp_str = range_convert_range (NULL, *pref->varp.range);
        label = g_strdup_printf ("%s: %s", pref->title, tmp_str);
        wmem_free(NULL, tmp_str);
        menu_item = gtk_menu_item_new_with_label(label);
        g_object_set_data (G_OBJECT(menu_item), "module", module);
        g_signal_connect(menu_item, "activate", G_CALLBACK(menu_prefs_edit_dlg), pref);
        g_free (label);
        break;
    case PREF_UAT:
        label = g_strdup_printf ("%s...", pref->title);
        menu_item = gtk_menu_item_new_with_label(label);
        g_signal_connect (menu_item, "activate", G_CALLBACK(uat_window_cb), pref->varp.uat);
        g_free (label);
        break;

    case PREF_COLOR:
    case PREF_CUSTOM:
        /* currently not supported */

    case PREF_STATIC_TEXT:
    case PREF_OBSOLETE:
    default:
        /* Nothing to add */
        return 0;
    }

    menu_preferences = gtk_ui_manager_get_widget(ui_menu, path);
    if(!menu_preferences)
        g_warning("menu_preferences Not found path");
    sub_menu = gtk_menu_item_get_submenu (GTK_MENU_ITEM(menu_preferences));
    gtk_menu_shell_append (GTK_MENU_SHELL(sub_menu), menu_item);
    gtk_widget_show (menu_item);

    return 0;
}

static guint
add_protocol_prefs_menu(pref_t *pref, gpointer data)
{
    return add_protocol_prefs_generic_menu(pref, data, ui_manager_tree_view_menu, "/TreeViewPopup/ProtocolPreferences");
}

static guint
add_protocol_prefs_packet_list_menu(pref_t *pref, gpointer data)
{
    return add_protocol_prefs_generic_menu(pref, data, ui_manager_packet_list_menu, "/PacketListMenuPopup/ProtocolPreferences");
}


static void
rebuild_protocol_prefs_menu(module_t *prefs_module_p, gboolean preferences,
        GtkUIManager *ui_menu, const char *path)
{
    GtkWidget *menu_preferences, *menu_item;
    GtkWidget *sub_menu;
    gchar     *label;

    menu_preferences = gtk_ui_manager_get_widget(ui_menu, path);
    if (prefs_module_p && preferences) {
        sub_menu = gtk_menu_new();
        gtk_menu_item_set_submenu (GTK_MENU_ITEM(menu_preferences), sub_menu);

        label = g_strdup_printf ("%s Preferences...", prefs_module_p->description);
        menu_item = gtk_image_menu_item_new_with_label(label);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM(menu_item),
                                       ws_gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU));
        gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
        g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
                                 G_CALLBACK(properties_cb), (GObject *) menu_item);
        gtk_widget_show(menu_item);
        g_free(label);

        menu_item = gtk_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(sub_menu), menu_item);
        gtk_widget_show(menu_item);

        if (ui_menu == ui_manager_tree_view_menu) {
            prefs_pref_foreach(prefs_module_p, add_protocol_prefs_menu, prefs_module_p);
        } else {
            prefs_pref_foreach(prefs_module_p, add_protocol_prefs_packet_list_menu, prefs_module_p);
        }
    } else {
        /* No preferences, remove sub menu */
        gtk_menu_item_set_submenu (GTK_MENU_ITEM(menu_preferences), NULL);
    }

}

static void
menu_visible_column_toggle (GtkWidget *w _U_, gpointer data)
{
    packet_list_toggle_visible_column (GPOINTER_TO_INT(data));
}

void
rebuild_visible_columns_menu (void)
{
    GtkWidget *menu_columns[2], *menu_item;
    GtkWidget *sub_menu;
    GList     *clp;
    fmt_data  *cfmt;
    gchar     *title;
    gint       i, col_id;

    menu_columns[0] = gtk_ui_manager_get_widget(ui_manager_main_menubar, "/Menubar/ViewMenu/DisplayedColumns");
    if(! menu_columns[0]){
        fprintf (stderr, "Warning: couldn't find menu_columns[0] path=/Menubar/ViewMenu/DisplayedColumns");
    }
    menu_columns[1] = gtk_ui_manager_get_widget(ui_manager_packet_list_heading, "/PacketListHeadingPopup/DisplayedColumns");
    /* Debug */
    if(! menu_columns[1]){
        fprintf (stderr, "Warning: couldn't find menu_columns[1] path=/PacketListHeadingPopup/DisplayedColumns");
    }

    for (i = 0; i < 2; i++) {
        sub_menu = gtk_menu_new();
        gtk_menu_item_set_submenu (GTK_MENU_ITEM(menu_columns[i]), sub_menu);

        clp = g_list_first (prefs.col_list);
        col_id = 0;
        while (clp) {
            cfmt = (fmt_data *) clp->data;
            if (cfmt->title[0]) {
                if (cfmt->fmt == COL_CUSTOM) {
                    title = g_strdup_printf ("%s  (%s)", cfmt->title, cfmt->custom_fields);
                } else {
                    title = g_strdup_printf ("%s  (%s)", cfmt->title, col_format_desc (cfmt->fmt));
                }
            } else {
                if (cfmt->fmt == COL_CUSTOM) {
                    title = g_strdup_printf ("(%s)", cfmt->custom_fields);
                } else {
                    title = g_strdup_printf ("(%s)", col_format_desc (cfmt->fmt));
                }
            }
            menu_item = gtk_check_menu_item_new_with_label(title);
            g_free (title);
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(menu_item), cfmt->visible);
            g_signal_connect(menu_item, "activate", G_CALLBACK(menu_visible_column_toggle), GINT_TO_POINTER(col_id));
            gtk_menu_shell_append (GTK_MENU_SHELL(sub_menu), menu_item);
            gtk_widget_show (menu_item);
            clp = g_list_next (clp);
            col_id++;
        }

        menu_item = gtk_separator_menu_item_new();
        gtk_menu_shell_append (GTK_MENU_SHELL(sub_menu), menu_item);
        gtk_widget_show (menu_item);

        menu_item = gtk_menu_item_new_with_label ("Display All");
        gtk_menu_shell_append (GTK_MENU_SHELL(sub_menu), menu_item);
        g_signal_connect(menu_item, "activate", G_CALLBACK(packet_list_heading_activate_all_columns_cb), NULL);
        gtk_widget_show (menu_item);
    }
}

void
menus_set_column_resolved (gboolean resolved, gboolean can_resolve)
{
    GtkWidget *menu;

    menu = gtk_ui_manager_get_widget(ui_manager_packet_list_heading, "/PacketListHeadingPopup/ShowResolved");
    if(!menu){
        fprintf (stderr, "Warning: couldn't find menu path=/PacketListHeadingPopup/ShowResolved");
    }
    g_object_set_data(G_OBJECT(menu), "skip-update", (void *)1);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu), resolved && can_resolve);
    set_menu_sensitivity(ui_manager_packet_list_heading, "/PacketListHeadingPopup/ShowResolved", can_resolve);
    g_object_set_data(G_OBJECT(menu), "skip-update", NULL);
}

void
menus_set_column_align_default (gboolean right_justify)
{
    GtkWidget   *submenu, *menu_item_child;
    GList       *child_list, *child_list_item;
    const gchar *menu_item_name;
    size_t       menu_item_len;

    /* get the submenu container item */
    submenu = gtk_ui_manager_get_widget (ui_manager_packet_list_heading, "/PacketListHeadingPopup");
    if(!submenu){
        fprintf (stderr, "Warning: couldn't find submenu path=/PacketListHeadingPopup");
    }

    /* find the corresponding menu items to update */
    child_list = gtk_container_get_children(GTK_CONTAINER(submenu));
    child_list_item = child_list;
    while (child_list_item) {
        menu_item_child = gtk_bin_get_child(GTK_BIN(child_list_item->data));
        if (menu_item_child != NULL) {
            menu_item_name = gtk_label_get_text(GTK_LABEL(menu_item_child));
            menu_item_len = strlen(menu_item_name);
            if(strncmp(menu_item_name, "Align Left", 10) == 0) {
                if (!right_justify && menu_item_len == 10) {
                    gtk_label_set_text(GTK_LABEL(menu_item_child), "Align Left\t(default)");
                } else if (right_justify && menu_item_len > 10) {
                    gtk_label_set_text(GTK_LABEL(menu_item_child), "Align Left");
                }
            } else if (strncmp (menu_item_name, "Align Right", 11) == 0) {
                if (right_justify && menu_item_len == 11) {
                    gtk_label_set_text(GTK_LABEL(menu_item_child), "Align Right\t(default)");
                } else if (!right_justify && menu_item_len > 11) {
                    gtk_label_set_text(GTK_LABEL(menu_item_child), "Align Right");
                }
            }
        }
        child_list_item = g_list_next(child_list_item);
    }
    g_list_free(child_list);
}

void
set_menus_for_selected_tree_row(capture_file *cf)
{
    gboolean properties;
    gint     id;

    if (cf->finfo_selected != NULL) {
        header_field_info *hfinfo = cf->finfo_selected->hfinfo;
        const char *abbrev;
        char *prev_abbrev;

        if (hfinfo->parent == -1) {
            abbrev = hfinfo->abbrev;
            id = (hfinfo->type == FT_PROTOCOL) ? proto_get_id((protocol_t *)hfinfo->strings) : -1;
        } else {
            abbrev = proto_registrar_get_abbrev(hfinfo->parent);
            id = hfinfo->parent;
        }
        properties = prefs_is_registered_protocol(abbrev);
        set_menu_sensitivity(ui_manager_tree_view_menu,
                             "/TreeViewPopup/GotoCorrespondingPacket", hfinfo->type == FT_FRAMENUM);
        set_menu_sensitivity(ui_manager_tree_view_menu,
                             "/TreeViewPopup/ShowPacketRefinNewWindow", hfinfo->type == FT_FRAMENUM);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/Copy",
                             TRUE);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/Copy/AsFilter",
                             proto_can_match_selected(cf->finfo_selected, cf->edt));
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/ApplyasColumn",
                             hfinfo->type != FT_NONE);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/ApplyAsFilter",
                             proto_can_match_selected(cf->finfo_selected, cf->edt));
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/PrepareaFilter",
                             proto_can_match_selected(cf->finfo_selected, cf->edt));
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/ColorizewithFilter",
                             proto_can_match_selected(cf->finfo_selected, cf->edt));
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/ProtocolPreferences",
                             properties);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/DisableProtocol",
                             (id == -1) ? FALSE : proto_can_toggle_protocol(id));
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/ExpandSubtrees",
                             cf->finfo_selected->tree_type != -1);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/CollapseSubtrees",
                             cf->finfo_selected->tree_type != -1);
#ifdef WANT_PACKET_EDITOR
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/EditPacket",
                             prefs.gui_packet_editor ? TRUE : FALSE);
#endif
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/WikiProtocolPage",
                             (id == -1) ? FALSE : TRUE);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/FilterFieldReference",
                             (id == -1) ? FALSE : TRUE);
        set_menu_sensitivity(ui_manager_main_menubar,
                             "/Menubar/FileMenu/ExportSelectedPacketBytes", TRUE);
        set_menu_sensitivity(ui_manager_main_menubar,
                             "/Menubar/GoMenu/GotoCorrespondingPacket", hfinfo->type == FT_FRAMENUM);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/Copy/Description",
                             proto_can_match_selected(cf->finfo_selected, cf->edt));
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/Copy/Fieldname",
                             proto_can_match_selected(cf->finfo_selected, cf->edt));
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/Copy/Value",
                             proto_can_match_selected(cf->finfo_selected, cf->edt));
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/Copy/AsFilter",
                             proto_can_match_selected(cf->finfo_selected, cf->edt));
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/AnalyzeMenu/ApplyasColumn",
                             hfinfo->type != FT_NONE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/AnalyzeMenu/ApplyAsFilter",
                             proto_can_match_selected(cf->finfo_selected, cf->edt));
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/AnalyzeMenu/PrepareaFilter",
                             proto_can_match_selected(cf->finfo_selected, cf->edt));
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/ExpandSubtrees",
                             cf->finfo_selected->tree_type != -1);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/CollapseSubtrees",
                             cf->finfo_selected->tree_type != -1);
        prev_abbrev = (char *)g_object_get_data(G_OBJECT(ui_manager_tree_view_menu), "menu_abbrev");
        if (!prev_abbrev || (strcmp (prev_abbrev, abbrev) != 0)) {
            /* No previous protocol or protocol changed - update Protocol Preferences menu */
            module_t *prefs_module_p = prefs_find_module(abbrev);
            rebuild_protocol_prefs_menu(prefs_module_p, properties, ui_manager_tree_view_menu, "/TreeViewPopup/ProtocolPreferences");

            g_object_set_data(G_OBJECT(ui_manager_tree_view_menu), "menu_abbrev", g_strdup(abbrev));
            g_free (prev_abbrev);
        }
    } else {
        set_menu_sensitivity(ui_manager_tree_view_menu,
                             "/TreeViewPopup/GotoCorrespondingPacket", FALSE);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/Copy", FALSE);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/ApplyasColumn", FALSE);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/ApplyAsFilter", FALSE);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/PrepareaFilter", FALSE);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/ColorizewithFilter", FALSE);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/ProtocolPreferences",
                             FALSE);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/DisableProtocol", FALSE);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/ExpandSubtrees", FALSE);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/CollapseSubtrees", FALSE);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/WikiProtocolPage",
                             FALSE);
        set_menu_sensitivity(ui_manager_tree_view_menu, "/TreeViewPopup/FilterFieldReference",
                             FALSE);
        set_menu_sensitivity(ui_manager_main_menubar,
                             "/Menubar/FileMenu/ExportSelectedPacketBytes", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar,
                             "/Menubar/GoMenu/GotoCorrespondingPacket", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/Copy/Description", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/Copy/Fieldname", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/Copy/Value", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/EditMenu/Copy/AsFilter", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/AnalyzeMenu/ApplyasColumn", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/AnalyzeMenu/ApplyAsFilter", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/AnalyzeMenu/PrepareaFilter", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/ExpandSubtrees", FALSE);
        set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/ViewMenu/CollapseSubtrees", FALSE);
    }
}

void set_menus_for_packet_history(gboolean back_history, gboolean forward_history)
{
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/GoMenu/Back", back_history);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/GoMenu/Forward", forward_history);
}


void set_menus_for_file_set(gboolean file_set, gboolean previous_file, gboolean next_file)
{
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/Set/ListFiles", file_set);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/Set/PreviousFile", previous_file);
    set_menu_sensitivity(ui_manager_main_menubar, "/Menubar/FileMenu/Set/NextFile", next_file);
}

GtkWidget *menus_get_profiles_rename_menu (void)
{
    return gtk_ui_manager_get_widget(ui_manager_statusbar_profiles_menu, "/ProfilesMenuPopup/Rename");
}

GtkWidget *menus_get_profiles_delete_menu (void)
{
    return gtk_ui_manager_get_widget(ui_manager_statusbar_profiles_menu, "/ProfilesMenuPopup/Delete");
}

GtkWidget *menus_get_profiles_change_menu (void)
{
    return gtk_ui_manager_get_widget(ui_manager_statusbar_profiles_menu, "/ProfilesMenuPopup/Change");
}

void set_menus_for_profiles(gboolean default_profile)
{
    set_menu_sensitivity(ui_manager_statusbar_profiles_menu, "/ProfilesMenuPopup/Rename", !default_profile);
    set_menu_sensitivity(ui_manager_statusbar_profiles_menu, "/ProfilesMenuPopup/Delete", !default_profile);
}

static void
ws_menubar_external_cb(GtkAction *action _U_, gpointer data _U_)
{
    ext_menubar_t *entry = NULL;
    gchar *url = NULL;

    if ( data != NULL )
    {
        entry = (ext_menubar_t *)data;
        if ( entry->type == EXT_MENUBAR_ITEM )
        {
            entry->callback(EXT_MENUBAR_GTK_GUI, (gpointer) ((void *)GTK_WINDOW(top_level)), entry->user_data);
        }
        else if ( entry->type == EXT_MENUBAR_URL )
        {
            url = (gchar *)entry->user_data;

            if(url != NULL)
                browser_open_url(url);
        }
    }
}

static void
ws_menubar_create_ui(ext_menu_t * menu, const char * xpath_parent, GtkActionGroup * action_group, gint depth)
{
    ext_menubar_t * item = NULL;
    GList * children = NULL;
    gchar * xpath, * submenu_xpath;
    GtkAction * menu_action;
    gchar *action_name;
    gchar ** paths = NULL;

    /* There must exists an xpath parent */
    g_assert(xpath_parent != NULL && strlen(xpath_parent) > 0);

    /* If the depth counter exceeds, something must have gone wrong */
    g_assert(depth < EXT_MENUBAR_MAX_DEPTH);

    /* Create a correct xpath, and just keep the necessary action ref [which will be paths [1]] */
    xpath = g_strconcat(xpath_parent, menu->name, NULL);

    /* Create the action for the menu item and add it to the action group */
    action_name = g_strconcat("/", menu->name, NULL);
    menu_action = (GtkAction *)g_object_new ( GTK_TYPE_ACTION,
            "name", action_name, "label", menu->label, NULL );
    g_free(action_name);

    gtk_action_group_add_action(action_group, menu_action);
    g_object_unref(menu_action);

    children = menu->children;

    /* Iterate children to create submenus */
    while ( children != NULL && children->data != NULL )
    {
        item = (ext_menubar_t *) children->data;

        if ( item->type == EXT_MENUBAR_MENU )
        {
            /* Handle Submenu entry */
            submenu_xpath = g_strconcat(xpath, "/", NULL);
            ws_menubar_create_ui(item, submenu_xpath, action_group, depth++);
            g_free(submenu_xpath);
        }
        else if ( item->type != EXT_MENUBAR_SEPARATOR )
        {
            action_name = g_strconcat("/", item->name, NULL);
            menu_action = (GtkAction*) g_object_new( GTK_TYPE_ACTION,
                    "name", action_name,
                    "label", item->label,
                    "tooltip", item->tooltip,
                    NULL);
            g_signal_connect(menu_action, "activate", G_CALLBACK(ws_menubar_external_cb), item );
            gtk_action_group_add_action(action_group, menu_action);
            g_object_unref(menu_action);
            g_free(action_name);

            /* Create the correct action path */
            paths = g_strsplit(xpath, "|", -1);

            /* Ensures that the above operation has not failed. If this fails, it is a major issue,
             * so an assertion is appropriate here */
            g_assert(paths != NULL && paths[1] != NULL && strlen(paths[1]) > 0);

            /* Handle a menu bar item. This cannot be done by register_menu_bar_menu_items, as it
             * will create it's own action group, assuming that the menu actions and submenu actions
             * have been pre-registered and globally defined names. This is not the case here. Also
             * register_menu_bar_menu_items adds a sorted list, completely destroying any sorting,
             * a plugin might have intended */
#if 0
            /* Left here as a reminder, that the code below does basically the same */
            register_menu_bar_menu_items( g_strdup(paths[1]), item->name, NULL, item->label,
                    NULL, item->tooltip, ws_menubar_external_cb, item, TRUE, NULL, NULL);
#endif

            /* Creating menu from entry */
            add_menu_item_to_main_menubar(g_strdup(paths[1]), item->name, NULL);
            g_strfreev(paths);
        }

        /* Iterate Loop */
        children = g_list_next(children);
    }

    /* Cleanup */
    g_free(xpath);
}

static void
ws_menubar_external_menus(void)
{
    GList * user_menu = NULL;
    ext_menu_t * menu = NULL;
    GtkActionGroup  *action_group = NULL;
    gchar groupdef[20], * xpath;
    guint8 cnt = 1;

    user_menu = ext_menubar_get_entries();

    while ( ( user_menu != NULL ) && ( user_menu->data != NULL ) )
    {
        menu = (ext_menu_t *) user_menu->data;

        /* On this level only menu items should exist. Not doing an assert here,
         * as it could be an honest mistake */
        if ( menu->type != EXT_MENUBAR_MENU )
        {
            user_menu = g_list_next(user_menu);
            continue;
        }

        /* Create unique main actiongroup name */
        g_snprintf(groupdef, 20, "UserDefined%02d", cnt);
        xpath = g_strconcat( "/MenuBar/", groupdef, "Menu|", NULL );

        /* Create an action group per menu */
        action_group = gtk_action_group_new(groupdef);

        /* Register action structure for each menu */
        gtk_ui_manager_insert_action_group(ui_manager_main_menubar, action_group, 0);

        /* Now we iterate over the items and add them to the UI. This has to be done after the action
         * group for this menu has been added, otherwise the actions will not be found */
        ws_menubar_create_ui(menu, xpath, action_group, 0 );

        /* Cleanup */
        g_free(xpath);
        g_object_unref(action_group);

        /* Iterate Loop */
        user_menu = g_list_next (user_menu);
        cnt++;
    }
}

void plugin_if_menubar_preference(gconstpointer user_data)
{
    if ( user_data != NULL )
    {
        GHashTable * dataSet = (GHashTable *) user_data;
        const char * module_name;
        const char * pref_name;
        const char * pref_value;
        if ( g_hash_table_lookup_extended(dataSet, "pref_module", NULL, (void**)&module_name ) &&
                g_hash_table_lookup_extended(dataSet, "pref_key", NULL, (void**)&pref_name ) &&
                g_hash_table_lookup_extended(dataSet, "pref_value", NULL, (void**)&pref_value ) )
        {
            if ( prefs_store_ext(module_name, pref_name, pref_value) )
            {
                redissect_packets();
                redissect_all_packet_windows();
            }
        }
    }
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
