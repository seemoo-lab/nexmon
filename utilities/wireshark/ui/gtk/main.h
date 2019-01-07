/* main.h
 * Global defines, etc.
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

#ifndef __MAIN_H__
#define __MAIN_H__

#include "globals.h"
#include "capture_opts.h"

/** @defgroup main_window_group Main window
 * The main window has the following submodules:
   @dot
  digraph main_dependencies {
      node [shape=record, fontname=Helvetica, fontsize=10];
      main [ label="main window" URL="\ref main.h"];
      menu [ label="menubar" URL="\ref menus.h"];
      toolbar [ label="toolbar" URL="\ref main_toolbar.h"];
      packet_list [ label="packet list pane" URL="\ref packet_list.h"];
      proto_draw [ label="packet details & bytes panes" URL="\ref main_proto_draw.h"];
      recent [ label="recent user settings" URL="\ref recent.h"];
      main -> menu [ arrowhead="open", style="solid" ];
      main -> toolbar [ arrowhead="open", style="solid" ];
      main -> packet_list [ arrowhead="open", style="solid" ];
      main -> proto_draw [ arrowhead="open", style="solid" ];
      main -> recent [ arrowhead="open", style="solid" ];
  }
  @enddot
 */

/** @file
 *  The main window, filter toolbar, program start/stop and a lot of other things
 *  @ingroup main_window_group
 *  @ingroup windows_group
 */

/** Global compile time version string */
extern void get_wireshark_gtk_compiled_info(GString *str);
extern void get_gui_compiled_info(GString *str);
/** Global runtime version string */
extern void get_wireshark_runtime_info(GString *str);


extern GtkWidget* wireless_tb;

void
airpcap_toolbar_encryption_cb(GtkWidget *entry, gpointer user_data);

/** User requested "Zoom In" by menu or toolbar.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void view_zoom_in_cb(GtkWidget *widget, gpointer data);

/** User requested "Zoom Out" by menu or toolbar.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void view_zoom_out_cb(GtkWidget *widget, gpointer data);

/** User requested "Zoom 100%" by menu or toolbar.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void view_zoom_100_cb(GtkWidget *widget, gpointer data);

/** User requested "Protocol Info" by ptree context menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void selected_ptree_info_cb(GtkWidget *widget, gpointer data);

/** User requested "Filter Reference" by ptree context menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void selected_ptree_ref_cb(GtkWidget *widget, gpointer data);

/** "Apply as Filter" / "Prepare a Filter" action type. */
typedef enum {
    MATCH_SELECTED_REPLACE, /**< "Selected" */
    MATCH_SELECTED_AND,     /**< "and Selected" */
    MATCH_SELECTED_OR,      /**< "or Selected" */
    MATCH_SELECTED_NOT,     /**< "Not Selected" */
    MATCH_SELECTED_AND_NOT, /**< "and not Selected" */
    MATCH_SELECTED_OR_NOT   /**< "or not Selected" */
} MATCH_SELECTED_E;

/** mask MATCH_SELECTED_E values (internally used) */
#define MATCH_SELECTED_MASK         0x0ff

/** "bitwise or" this with MATCH_SELECTED_E value for instant apply instead of prepare only */
#define MATCH_SELECTED_APPLY_NOW    0x100

/** "bitwise or" this with MATCH_SELECTED_E value for copy to clipboard instead of prepare only */
#define MATCH_SELECTED_COPY_ONLY    0x200

/** User requested one of "Apply as Filter" or "Prepare a Filter" functions
 *  by menu or context menu of protocol tree.
 *
 * @param data parent widget
 * @param action the function to use
 */
extern void match_selected_ptree_cb(gpointer data, MATCH_SELECTED_E action);

/** "Copy ..." action type. */
typedef enum {
    COPY_SELECTED_DESCRIPTION,  /**< "Copy Description" */
    COPY_SELECTED_FIELDNAME,    /**< "Copy Fieldname" */
    COPY_SELECTED_VALUE         /**< "Copy Value" */
} COPY_SELECTED_E;

/** User highlighted item in details window and then right clicked and selected the copy option
 *
 * @param w parent widget
 * @param data parent widget
 * @param action the function to use
 */
extern void copy_selected_plist_cb(GtkWidget *w _U_, gpointer data, COPY_SELECTED_E action);

/** Set or remove a time reference on this frame
 *
 * @param set TRUE = set time ref, FALSE=unset time ref
 * @param frame pointer to frame
 * @param row row number
 */
extern void set_frame_reftime(gboolean set, frame_data *frame, gint row);

/** User requested the colorize function
 *  by menu or context menu of protocol tree.
 *
 * @param w parent widget
 * @param data parent widget
 * @param filt_nr  1-10: use filter for color 1-10
 *                    0: open new colorization rule dialog
 *                  255: clear filters for color 1-10
 */
extern void colorize_selected_ptree_cb(GtkWidget *w, gpointer data, guint8 filt_nr);

/** User requested one of "Apply as Filter" or "Prepare a Filter" functions
 *  by context menu of packet list.
 *
 * @param data parent widget
 * @param action the function to use
 */
extern void match_selected_plist_cb(gpointer data, MATCH_SELECTED_E action);

/** User requested "Quit" by menu or toolbar.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void file_quit_cmd_cb(GtkWidget *widget, gpointer data);

/** User requested "Print" by menu or toolbar.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void file_print_cmd_cb(GtkWidget *widget, gpointer data);

/** User requested "Print" by packet list context menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void file_print_selected_cmd_cb(GtkWidget *widget _U_, gpointer data _U_);

/** User requested "Export as Plain Text" by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void export_text_cmd_cb(GtkWidget *widget, gpointer data);

/** User requested "Export as Postscript" by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void export_ps_cmd_cb(GtkWidget *widget, gpointer data);

/** User requested "Export as PSML" by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void export_psml_cmd_cb(GtkWidget *widget, gpointer data);

/** User requested "Export as PDML" by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void export_pdml_cmd_cb(GtkWidget *widget, gpointer data);

/** User requested "Export as CSV" by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void export_csv_cmd_cb(GtkWidget *widget, gpointer data);

/** User requested "Export as C Arrays" by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void export_carrays_cmd_cb(GtkWidget *widget, gpointer data);

/** User requested "Export as JSON" by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void export_json_cmd_cb(GtkWidget *widget, gpointer data);

/** User requested "Expand Tree" by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void expand_tree_cb(GtkWidget *widget, gpointer data);

/** User requested "Collapse Tree" by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void collapse_tree_cb(GtkWidget *widget, gpointer data);

/** User requested "Expand All" by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void expand_all_cb(GtkWidget *widget, gpointer data);

/** User requested "Apply as a custom column" by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void apply_as_custom_column_cb(GtkWidget *widget, gpointer data);

/** User requested "Collapse All" by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void collapse_all_cb(GtkWidget *widget, gpointer data);

/** User requested "Resolve Name" by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 */
extern void resolve_name_cb(GtkWidget *widget, gpointer data);

/** Action to take for reftime_frame_cb() */
typedef enum {
    REFTIME_TOGGLE,     /**< toggle ref frame */
    REFTIME_FIND_NEXT,  /**< find next ref frame */
    REFTIME_FIND_PREV   /**< find previous ref frame */
} REFTIME_ACTION_E;

/** User requested one of the "Time Reference" functions by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 * @param action the function to use
 */
extern void reftime_frame_cb(GtkWidget *widget, gpointer data, REFTIME_ACTION_E action);

/** User requested the "Find Next Mark" function by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 * @param action unused
 */
extern void find_next_mark_cb(GtkWidget *widget, gpointer data, int action);

/** User requested the "Find Previous Mark" function by menu.
 *
 * @param widget parent widget (unused)
 * @param data unused
 * @param action unused
 */
extern void find_prev_mark_cb(GtkWidget *widget, gpointer data, int action);

#if 0
/** Empty out the combobox entry field */
extern void dfilter_combo_add_empty(void);
#endif

/** Update various parts of the main window for a capture file "unsaved
 *  changes" change.
 *
 * @param cf capture_file structure for the capture file.
 */
extern void main_update_for_unsaved_changes(capture_file *cf);

#ifdef HAVE_LIBPCAP
/** Update various parts of the main window for a change in whether
 * "auto scroll in live capture" is on or off.
 *
 * @param auto_scroll_live_in new state of "auto scroll in live capture"
 */
void main_auto_scroll_live_changed(gboolean auto_scroll_live_in);
#endif

/** Update parts of the main window for a change in colorization. */
extern void main_colorize_changed(gboolean packet_list_colorize);

/** Quit the program.
 *
 * @return TRUE, if a file read is in progress
 */
extern gboolean main_do_quit(void);

/** Rearrange the main window widgets, user changed its preferences. */
extern void main_widgets_rearrange(void);

/** Show or hide the main window widgets, user changed its preferences. */
extern void main_widgets_show_or_hide(void);

/* Update main window items based on whether we have a packet history. */
extern void main_set_for_packet_history(gboolean back_history, gboolean forward_history);

/** Apply a new filter string.
 *  Call cf_filter_packets() and add this filter string to the recent filter list.
 *
 * @param cf the capture file
 * @param dftext the new filter string
 * @param force force the refiltering, even if filter string doesn't changed
 * @return TRUE, if the filtering succeeded
 */
extern gboolean main_filter_packets(capture_file *cf, const gchar *dftext,
    gboolean force);

#ifdef _WIN32
/** Win32 only: Create a console. Beware: cannot be closed again. */
extern void create_console(void);
#endif

/** Change configuration profile */
extern void change_configuration_profile(const gchar *profile_name);

/** Update GUI for changes in fields */
extern void main_fields_changed (void);

/** redissect packets and update UI */
extern void redissect_packets(void);

/** Fetch all IP addresses from selected row */
extern GList *get_ip_address_list_from_packet_list_row(gpointer data);

extern GtkWidget *pkt_scrollw;

#endif /* __MAIN_H__ */
