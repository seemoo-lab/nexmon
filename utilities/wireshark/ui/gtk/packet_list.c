/* packet_list.c
 * Routines to implement a new GTK2 packet list using our custom model
 * Copyright 2008-2009, Stephen Fisher (see AUTHORS file)
 * Co-authors Anders Broman, Kovarththanan Rajaratnam and Stig Bjorlykke.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <epan/prefs.h>
#include <epan/packet.h>
#include <epan/column.h>
#include <epan/strutil.h>
#include <epan/plugin_if.h>

#include "ui/main_statusbar.h"
#include "ui/packet_list_utils.h"
#include "ui/preference_utils.h"
#include "ui/recent.h"
#include "ui/recent_utils.h"

#include "packet_list_store.h"
#include "ui/gtk/packet_list.h"
#include "ui/gtk/gtkglobals.h"
#include "ui/gtk/font_utils.h"
#include "ui/gtk/packet_history.h"
#include "ui/gtk/keys.h"
#include "ui/gtk/menus.h"
#include <epan/color_filters.h>
#include "ui/gtk/color_utils.h"
#include "ui/gtk/packet_win.h"
#include "ui/gtk/main.h"
#include "ui/gtk/dlg_utils.h"
#include "ui/gtk/filter_dlg.h"
#include "ui/gtk/filter_autocomplete.h"

#include "ui/gtk/old-gtk-compat.h"

#include <wsutil/str_util.h>

#define COLUMN_WIDTH_MIN 40
#define MAX_COMMENTS_TO_FETCH 20000000 /* Arbitrary */

#define COL_EDIT_COLUMN          "column"
#define COL_EDIT_FORMAT_CMB      "format_cmb"
#define COL_EDIT_TITLE_TE        "title_te"
#define COL_EDIT_FIELD_LB        "field_lb"
#define COL_EDIT_FIELD_TE        "field_te"
#define COL_EDIT_OCCURRENCE_LB   "occurrence_lb"
#define COL_EDIT_OCCURRENCE_TE   "occurrente_te"

static PacketList *packetlist;
static gboolean last_at_end = FALSE;
static gboolean enable_color;
static gulong column_changed_handler_id;

static GtkWidget *create_view_and_model(void);
static void scroll_to_and_select_iter(GtkTreeModel *model, GtkTreeSelection *selection, GtkTreeIter *iter);
static void packet_list_select_cb(GtkTreeView *tree_view, gpointer data _U_);
static void packet_list_double_click_cb(GtkTreeView *treeview,
					    GtkTreePath *path _U_,
					    GtkTreeViewColumn *col _U_,
					    gpointer userdata _U_);
static void show_cell_data_func(GtkTreeViewColumn *col,
				GtkCellRenderer *renderer,
				GtkTreeModel *model,
				GtkTreeIter *iter,
				gpointer data);
static gint row_number_from_iter(GtkTreeIter *iter);
static void scroll_to_current(void);
static gboolean query_packet_list_tooltip_cb(GtkWidget *widget, gint x, gint y, gboolean keyboard_tip, GtkTooltip *tooltip, gpointer data _U_);

GtkWidget *
packet_list_create(void)
{
	GtkWidget *view, *scrollwin;

	scrollwin = scrolled_window_new(NULL, NULL);

	view = create_view_and_model();

	gtk_container_add(GTK_CONTAINER(scrollwin), view);

	g_object_set_data(G_OBJECT(popup_menu_object), E_MPACKET_LIST_KEY, view);

	return scrollwin;
}

/** @todo XXX - implement a smarter solution for recreating the packet list */
void
packet_list_recreate(void)
{
	g_signal_handler_block(packetlist->view, column_changed_handler_id);
	gtk_widget_destroy(pkt_scrollw);

	prefs.num_cols = g_list_length(prefs.col_list);

	col_cleanup(&cfile.cinfo);
	build_column_format_array(&cfile.cinfo, prefs.num_cols, FALSE);

	pkt_scrollw = packet_list_create();
	gtk_widget_show_all(pkt_scrollw);

	main_widgets_rearrange();

	if(cfile.state != FILE_CLOSED)
		redissect_packets();
}

guint
packet_list_append(column_info *cinfo _U_, frame_data *fdata)
{
	/* fdata should be filled with the stuff we need
	 * strings are built at display time.
	 */
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(packetlist->view));
	guint visible_pos = packet_list_append_record(packetlist, fdata);
	if(model)
		/* If the model is connected there is no packetsbar_update from "thaw */
		packets_bar_update();
	/* Return the _visible_ position */

	return visible_pos;
}

static void
col_title_change_ok (GtkWidget *w, gpointer parent_w)
{
	GtkTreeViewColumn *col;
	const gchar  *title, *name, *occurrence_text;
	gint          col_id, cur_fmt, occurrence, col_width;
	gchar        *escaped_title;
	gboolean      recreate = FALSE;

	col = (GtkTreeViewColumn *)g_object_get_data (G_OBJECT(w), COL_EDIT_COLUMN);
	col_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(col), E_MPACKET_LIST_COL_KEY));

	title = gtk_entry_get_text(GTK_ENTRY(g_object_get_data (G_OBJECT(w), COL_EDIT_TITLE_TE)));
	name = gtk_entry_get_text(GTK_ENTRY(g_object_get_data (G_OBJECT(w), COL_EDIT_FIELD_TE)));
	cur_fmt =  gtk_combo_box_get_active(GTK_COMBO_BOX(g_object_get_data (G_OBJECT(w), COL_EDIT_FORMAT_CMB)));
	occurrence_text = gtk_entry_get_text(GTK_ENTRY(g_object_get_data (G_OBJECT(w), COL_EDIT_OCCURRENCE_TE)));
	occurrence = (gint)strtol(occurrence_text, NULL, 10);

	escaped_title = ws_strdup_escape_char(title, '_');
	gtk_tree_view_column_set_title(col, escaped_title);
	g_free(escaped_title);

	if (strcmp (title, get_column_title(col_id)) != 0) {
		set_column_title (col_id, title);
	}

	if (cur_fmt != get_column_format(col_id)) {
		set_column_format (col_id, cur_fmt);
		recreate = TRUE;
	}

	if (cur_fmt == COL_CUSTOM) {
		const gchar *custom_fields = get_column_custom_fields(col_id);
		if ((custom_fields && strcmp (name, custom_fields) != 0) || (custom_fields == NULL)) {
			set_column_custom_fields(col_id, name);
			recreate = TRUE;
		}

		if (occurrence != get_column_custom_occurrence(col_id)) {
			set_column_custom_occurrence (col_id, occurrence);
			recreate = TRUE;
		}
	}

	col_width = get_default_col_size (packetlist->view, title);
	gtk_tree_view_column_set_min_width(col, col_width);

	if (!prefs.gui_use_pref_save) {
		prefs_main_write();
	}

	rebuild_visible_columns_menu ();

	if (recreate) {
		packet_list_recreate();
	}

	packet_list_resize_column (col_id);
	window_destroy(GTK_WIDGET(parent_w));
}

static void
col_title_change_cancel (GtkWidget *w _U_, gpointer parent_w)
{
	window_destroy(GTK_WIDGET(parent_w));
}

static void
col_details_format_changed_cb(GtkWidget *w, gpointer data _U_)
{
	GtkWidget *field_lb, *field_te, *occurrence_lb, *occurrence_te;
	gint       cur_fmt;

	field_lb = (GtkWidget *)g_object_get_data (G_OBJECT(w), COL_EDIT_FIELD_LB);
	field_te = (GtkWidget *)g_object_get_data (G_OBJECT(w), COL_EDIT_FIELD_TE);
	occurrence_lb = (GtkWidget *)g_object_get_data (G_OBJECT(w), COL_EDIT_OCCURRENCE_LB);
	occurrence_te = (GtkWidget *)g_object_get_data (G_OBJECT(w), COL_EDIT_OCCURRENCE_TE);

	cur_fmt = gtk_combo_box_get_active(GTK_COMBO_BOX(w));

	if (cur_fmt == COL_CUSTOM) {
		/* Changing to custom */
		gtk_widget_set_sensitive(field_lb, TRUE);
		gtk_widget_set_sensitive(field_te, TRUE);
		gtk_widget_set_sensitive(occurrence_lb, TRUE);
		gtk_widget_set_sensitive(occurrence_te, TRUE);
	} else {
		/* Changing to non-custom */
		gtk_widget_set_sensitive(field_lb, FALSE);
		gtk_widget_set_sensitive(field_te, FALSE);
		gtk_widget_set_sensitive(occurrence_lb, FALSE);
		gtk_widget_set_sensitive(occurrence_te, FALSE);
	}
}

static void
col_details_edit_dlg (gint col_id, GtkTreeViewColumn *col)
{
	const gchar *title = gtk_tree_view_column_get_title(col);
	gchar *unescaped_title = ws_strdup_unescape_char(title, '_');

	GtkWidget *label, *field_lb, *occurrence_lb;
	GtkWidget *title_te, *format_cmb, *field_te, *occurrence_te;
	GtkWidget *win, *main_grid, *main_vb, *bbox, *cancel_bt, *ok_bt;
	char       custom_occurrence_str[8];
	gint       cur_fmt, i;

	win = dlg_window_new("Wireshark: Edit Column Details");

	gtk_window_set_resizable(GTK_WINDOW(win),FALSE);
	gtk_window_resize(GTK_WINDOW(win), 400, 100);

	main_vb = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 6, FALSE);
	gtk_container_add(GTK_CONTAINER(win), main_vb);
	gtk_container_set_border_width(GTK_CONTAINER(main_vb), 6);

	main_grid = ws_gtk_grid_new();
	gtk_box_pack_start(GTK_BOX(main_vb), main_grid, FALSE, FALSE, 0);
	ws_gtk_grid_set_column_spacing(GTK_GRID(main_grid), 10);
	ws_gtk_grid_set_row_spacing(GTK_GRID(main_grid), 5);

	label = gtk_label_new("Title:");
	ws_gtk_grid_attach_defaults(GTK_GRID(main_grid), label, 0, 0, 1, 1);
	gtk_misc_set_alignment(GTK_MISC(label), 1.0f, 0.5f);
	gtk_widget_set_tooltip_text(label, "Packet list column title.");

	title_te = gtk_entry_new();
	ws_gtk_grid_attach_defaults(GTK_GRID(main_grid), title_te, 1, 0, 1, 1);
	gtk_entry_set_text(GTK_ENTRY(title_te), unescaped_title);
	g_free(unescaped_title);
	gtk_widget_set_tooltip_text(title_te, "Packet list column title.");

	label = gtk_label_new("Field type:");
	ws_gtk_grid_attach_defaults(GTK_GRID(main_grid), label, 0, 1, 1, 1);
	gtk_misc_set_alignment(GTK_MISC(label), 1.0f, 0.5f);
	gtk_widget_set_tooltip_text(label, "Select which packet information to present in the column.");

	format_cmb = gtk_combo_box_text_new();
	for (i = 0; i < NUM_COL_FMTS; i++) {
	   gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(format_cmb), col_format_desc(i));
	}
	g_signal_connect(format_cmb, "changed", G_CALLBACK(col_details_format_changed_cb), NULL);
	ws_gtk_grid_attach_defaults(GTK_GRID(main_grid), format_cmb, 1, 1, 1, 1);
	gtk_widget_set_tooltip_text(format_cmb, "Select which packet information to present in the column.");

	field_lb = gtk_label_new("Field name:");
	ws_gtk_grid_attach_defaults(GTK_GRID(main_grid), field_lb, 0, 2, 1, 1);
	gtk_misc_set_alignment(GTK_MISC(field_lb), 1.0f, 0.5f);
	gtk_widget_set_tooltip_text(field_lb,
			      "Field name used when field type is \"Custom\". "
			      "This string has the same syntax as a display filter string.");
	field_te = gtk_entry_new();
	ws_gtk_grid_attach_defaults(GTK_GRID(main_grid), field_te, 1, 2, 1, 1);
	g_object_set_data (G_OBJECT(field_te), E_FILT_MULTI_FIELD_NAME_ONLY_KEY, (gpointer)"");
	g_signal_connect(field_te, "changed", G_CALLBACK(filter_te_syntax_check_cb), NULL);
	g_signal_connect(field_te, "key-press-event", G_CALLBACK (filter_string_te_key_pressed_cb), NULL);
	g_signal_connect(win, "key-press-event", G_CALLBACK (filter_parent_dlg_key_pressed_cb), NULL);
	gtk_widget_set_tooltip_text(field_te,
			      "Field names used when field type is \"Custom\". "
			      "This string has the same syntax as a display filter string.");

	occurrence_lb = gtk_label_new("Occurrence:");
	ws_gtk_grid_attach_defaults(GTK_GRID(main_grid), occurrence_lb, 0, 3, 1, 1);
	gtk_misc_set_alignment(GTK_MISC(occurrence_lb), 1.0f, 0.5f);
	gtk_widget_set_tooltip_text (occurrence_lb,
			      "Field occurrence to use. "
			      "0=all (default), 1=first, 2=second, ..., -1=last.");

	occurrence_te = gtk_entry_new();
	gtk_entry_set_max_length (GTK_ENTRY(occurrence_te), 4);
	ws_gtk_grid_attach_defaults(GTK_GRID(main_grid), occurrence_te, 1, 3, 1, 1);
	gtk_widget_set_tooltip_text (occurrence_te,
			      "Field occurrence to use. "
			      "0=all (default), 1=first, 2=second, ..., -1=last.");

	bbox = dlg_button_row_new(GTK_STOCK_CANCEL,GTK_STOCK_OK, NULL);
	gtk_box_pack_end(GTK_BOX(main_vb), bbox, FALSE, FALSE, 0);

	ok_bt = (GtkWidget *)g_object_get_data(G_OBJECT(bbox), GTK_STOCK_OK);
	g_object_set_data (G_OBJECT(ok_bt), COL_EDIT_COLUMN, col);
	g_object_set_data (G_OBJECT(ok_bt), COL_EDIT_FORMAT_CMB, format_cmb);
	g_object_set_data (G_OBJECT(ok_bt), COL_EDIT_TITLE_TE, title_te);
	g_object_set_data (G_OBJECT(ok_bt), COL_EDIT_FIELD_TE, field_te);
	g_object_set_data (G_OBJECT(ok_bt), COL_EDIT_OCCURRENCE_TE, occurrence_te);
	g_object_set_data (G_OBJECT(format_cmb), COL_EDIT_FIELD_LB, field_lb);
	g_object_set_data (G_OBJECT(format_cmb), COL_EDIT_FIELD_TE, field_te);
	g_object_set_data (G_OBJECT(format_cmb), COL_EDIT_OCCURRENCE_LB, occurrence_lb);
	g_object_set_data (G_OBJECT(format_cmb), COL_EDIT_OCCURRENCE_TE, occurrence_te);
	g_signal_connect(ok_bt, "clicked", G_CALLBACK(col_title_change_ok), win);

	cur_fmt = get_column_format (col_id);
	gtk_combo_box_set_active(GTK_COMBO_BOX(format_cmb), cur_fmt);
	if (cur_fmt == COL_CUSTOM) {
		gtk_entry_set_text(GTK_ENTRY(field_te), get_column_custom_fields(col_id));
		g_snprintf(custom_occurrence_str, sizeof(custom_occurrence_str), "%d", get_column_custom_occurrence(col_id));
		gtk_entry_set_text(GTK_ENTRY(occurrence_te), custom_occurrence_str);
	}

	dlg_set_activate(title_te, ok_bt);
	dlg_set_activate(field_te, ok_bt);
	dlg_set_activate(occurrence_te, ok_bt);

	cancel_bt = (GtkWidget *)g_object_get_data(G_OBJECT(bbox), GTK_STOCK_CANCEL);
	g_signal_connect(cancel_bt, "clicked", G_CALLBACK(col_title_change_cancel), win);
	window_set_cancel_button(win, cancel_bt, NULL);

	gtk_widget_grab_default(ok_bt);
	gtk_widget_show_all(win);
}

/* Process sort request;
 *   order:          GTK_SORT_ASCENDING or GTK_SORT_DESCENDING
 *   sort_indicator: TRUE: set sort_indicator on column; FALSE: don't set ....
 *
 * If necessary, columns are first "columnized" for all rows in the packet-list; If this
 *  is not completed (i.e., stopped), then the sort request is aborted.
 */
static void
packet_list_sort_column (gint col_id, GtkTreeViewColumn *col, GtkSortType order, gboolean sort_indicator)
{
	GtkTreeViewColumn *prev_col;

	if (col == NULL) {
		col = gtk_tree_view_get_column(GTK_TREE_VIEW(packetlist->view), col_id);
	}
	g_assert(col);

	if (!packet_list_do_packet_list_dissect_and_cache_all(packetlist, col_id)) {
		return;  /* "stopped": do not try to sort */
	}

	prev_col = (GtkTreeViewColumn *)
	  g_object_get_data(G_OBJECT(packetlist->view), E_MPACKET_LIST_PREV_COLUMN_KEY);

	if (prev_col) {
		gtk_tree_view_column_set_sort_indicator(prev_col, FALSE);
	}
	gtk_tree_view_column_set_sort_indicator(col, sort_indicator);
	gtk_tree_view_column_set_sort_order (col, order);
	g_object_set_data(G_OBJECT(packetlist->view), E_MPACKET_LIST_PREV_COLUMN_KEY, col);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(packetlist), col_id, order); /* triggers sort callback */

	scroll_to_current ();
}

/*
 * We have our own functionality to toggle sort order on a column to avoid
 * having empty sorting arrow widgets in the column header.
 */
static void
packet_list_column_clicked_cb (GtkTreeViewColumn *col, gpointer user_data _U_)
{
	GtkSortType order = gtk_tree_view_column_get_sort_order (col);
	gint col_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(col), E_MPACKET_LIST_COL_KEY));

	if (cfile.state == FILE_READ_IN_PROGRESS)
		return;

	if (!gtk_tree_view_column_get_sort_indicator(col)) {
		packet_list_sort_column (col_id, col, GTK_SORT_ASCENDING, TRUE);
	} else if (order == GTK_SORT_ASCENDING) {
		packet_list_sort_column (col_id, col, GTK_SORT_DESCENDING, TRUE);
	} else {
		packet_list_sort_column (0, NULL, GTK_SORT_ASCENDING, FALSE);
	}
}

static gdouble
get_xalign_value (gchar xalign, gboolean right_justify)
{
	double value;

	switch (xalign) {
	case COLUMN_XALIGN_RIGHT:
		value = 1.0f;
		break;
	case COLUMN_XALIGN_CENTER:
		value = 0.5f;
		break;
	case COLUMN_XALIGN_LEFT:
		value = 0.0f;
		break;
	case COLUMN_XALIGN_DEFAULT:
	default:
		if (right_justify) {
			value = 1.0f;
		} else {
			value = 0.0f;
		}
		break;
	}

	return value;
}

static void
packet_list_xalign_column (gint col_id, GtkTreeViewColumn *col, gchar xalign)
{
	GList *renderers = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT(col));
	gboolean right_justify = right_justify_column(col_id, &cfile);
	gdouble value = get_xalign_value (xalign, right_justify);
	GList *entry;
	GtkCellRenderer *renderer;

	entry = g_list_first(renderers);
	while (entry) {
		renderer = (GtkCellRenderer *)entry->data;
		g_object_set(G_OBJECT(renderer), "xalign", value, NULL);
		entry = g_list_next (entry);
	}
	g_list_free (renderers);

	if ((xalign == COLUMN_XALIGN_LEFT && !right_justify) ||
	    (xalign == COLUMN_XALIGN_RIGHT && right_justify)) {
		/* Default value selected, save default in the recent settings */
		xalign = COLUMN_XALIGN_DEFAULT;
	}

	recent_set_column_xalign (col_id, xalign);
	gtk_widget_queue_draw (packetlist->view);
}

static void
packet_list_set_visible_column (gint col_id, GtkTreeViewColumn *col, gboolean visible)
{
	gtk_tree_view_column_set_visible(col, visible);
	set_column_visible(col_id, visible);

	if (!prefs.gui_use_pref_save) {
		prefs_main_write();
	}

	rebuild_visible_columns_menu ();
	gtk_widget_queue_draw (packetlist->view);
}

void
packet_list_toggle_visible_column (gint col_id)
{
	GtkTreeViewColumn *col =
	  gtk_tree_view_get_column(GTK_TREE_VIEW(packetlist->view), col_id);

	packet_list_set_visible_column (col_id, col, get_column_visible(col_id) ? FALSE : TRUE);
}

void
packet_list_set_all_columns_visible (void)
{
	GtkTreeViewColumn *col;
	int col_id;

	for (col_id = 0; col_id < cfile.cinfo.num_cols; col_id++) {
		col = gtk_tree_view_get_column(GTK_TREE_VIEW(packetlist->view), col_id);
		gtk_tree_view_column_set_visible(col, TRUE);
		set_column_visible(col_id, TRUE);
	}

	if (!prefs.gui_use_pref_save) {
		prefs_main_write();
	}

	rebuild_visible_columns_menu ();
	gtk_widget_queue_draw (packetlist->view);
}

static void
packet_list_remove_column (gint col_id, GtkTreeViewColumn *col _U_)
{
	column_prefs_remove_nth(col_id);

	if (!prefs.gui_use_pref_save) {
		prefs_main_write();
	}

	packet_list_recreate();
}

static void
packet_list_toggle_resolved (GtkWidget *w, gint col_id)
{
	/* We have to check for skip-update because we get an emit in menus_set_column_resolved() */
	if (g_object_get_data(G_OBJECT(w), "skip-update") == NULL) {
		set_column_resolved (col_id, get_column_resolved (col_id) ? FALSE : TRUE);

		if (!prefs.gui_use_pref_save) {
			prefs_main_write();
		}

		packet_list_recreate();
	}
}

void
packet_list_column_menu_cb (GtkWidget *w, gpointer user_data _U_, COLUMN_SELECTED_E action)
{
	GtkTreeViewColumn *col = (GtkTreeViewColumn *)
	  g_object_get_data(G_OBJECT(packetlist->view), E_MPACKET_LIST_COLUMN_KEY);
	gint col_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(col), E_MPACKET_LIST_COL_KEY));

	switch (action) {
	case COLUMN_SELECTED_SORT_ASCENDING:
		packet_list_sort_column (col_id, col, GTK_SORT_ASCENDING, TRUE);
		break;
	case COLUMN_SELECTED_SORT_DESCENDING:
		packet_list_sort_column (col_id, col, GTK_SORT_DESCENDING, TRUE);
		break;
	case COLUMN_SELECTED_SORT_NONE:
		packet_list_sort_column (0, NULL, GTK_SORT_ASCENDING, FALSE);
		break;
	case COLUMN_SELECTED_TOGGLE_RESOLVED:
		packet_list_toggle_resolved (w, col_id);
		break;
	case COLUMN_SELECTED_ALIGN_LEFT:
		packet_list_xalign_column (col_id, col, COLUMN_XALIGN_LEFT);
		break;
	case COLUMN_SELECTED_ALIGN_CENTER:
		packet_list_xalign_column (col_id, col, COLUMN_XALIGN_CENTER);
		break;
	case COLUMN_SELECTED_ALIGN_RIGHT:
		packet_list_xalign_column (col_id, col, COLUMN_XALIGN_RIGHT);
		break;
	case COLUMN_SELECTED_ALIGN_DEFAULT:
		packet_list_xalign_column (col_id, col, COLUMN_XALIGN_DEFAULT);
		break;
	case COLUMN_SELECTED_RESIZE:
		packet_list_resize_column (col_id);
		break;
	case COLUMN_SELECTED_CHANGE:
		col_details_edit_dlg (col_id, col);
		break;
	case COLUMN_SELECTED_HIDE:
		packet_list_set_visible_column (col_id, col, FALSE);
		break;
	case COLUMN_SELECTED_REMOVE:
		packet_list_remove_column (col_id, col);
		break;
	default:
		g_assert_not_reached();
		break;
	}
}

static gboolean
packet_list_column_button_pressed_cb (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	GtkWidget *col = (GtkWidget *) data;
	GtkWidget *menu = (GtkWidget *)g_object_get_data(G_OBJECT(popup_menu_object), PM_PACKET_LIST_COL_KEY);
	gint       col_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(col), E_MPACKET_LIST_COL_KEY));
	gboolean   right_justify = right_justify_column (col_id, &cfile);

	menus_set_column_align_default (right_justify);
	menus_set_column_resolved (get_column_resolved (col_id), resolve_column (col_id, &cfile));
	g_object_set_data(G_OBJECT(packetlist->view), E_MPACKET_LIST_COLUMN_KEY, col);
	return popup_menu_handler (widget, event, menu);
}

static gboolean packet_list_recreate_delayed(gpointer user_data _U_)
{
	packet_list_recreate();
	return FALSE;
}

static void
column_dnd_changed_cb(GtkTreeView *tree_view, gpointer data _U_)
{
	GtkTreeViewColumn  *column;
	GtkTreeSelection   *selection;
	GtkTreeModel  *model;
	GtkTreeIter    iter;
	GList         *columns, *list, *clp, *new_col_list = NULL;
	gint           old_col_id, new_col_id = 0;
	fmt_data      *cfmt;

	selection = gtk_tree_view_get_selection(tree_view);
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	list = columns = gtk_tree_view_get_columns(tree_view);
	while (columns) {
		column = (GtkTreeViewColumn *)columns->data;
		old_col_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(column), E_MPACKET_LIST_COL_KEY));

		clp = g_list_nth (prefs.col_list, old_col_id);
		cfmt = (fmt_data *) clp->data;
		new_col_list = g_list_append (new_col_list, cfmt);
		columns = g_list_next (columns);
		new_col_id++;
	}
	g_list_free (list);
	g_list_free (prefs.col_list);

	prefs.col_list = new_col_list;

	if (!prefs.gui_use_pref_save) {
		prefs_main_write();
	}

	/* The columns widget is part of the packets list, delay destruction to
	 * avoid triggering a use-after-free (maybe a GTK3 bug?) */
	g_idle_add(packet_list_recreate_delayed, NULL);
}

static GtkWidget *
create_view_and_model(void)
{
	GtkTreeViewColumn *col;
	GtkCellRenderer *renderer;
	gint i, col_width;
	gdouble value;
	gchar *tooltip_text;
	gint col_min_width;
	gchar *escaped_title;
	col_item_t* col_item;

	packetlist = packet_list_new();

	packetlist->view = tree_view_new(GTK_TREE_MODEL(packetlist));

	gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(packetlist->view),
						TRUE);
	g_signal_connect(packetlist->view, "cursor-changed",
			 G_CALLBACK(packet_list_select_cb), NULL);
	g_signal_connect(packetlist->view, "row-activated",
			 G_CALLBACK(packet_list_double_click_cb), NULL);
	g_signal_connect(packetlist->view, "button_press_event", G_CALLBACK(popup_menu_handler),
				   g_object_get_data(G_OBJECT(popup_menu_object), PM_PACKET_LIST_KEY));
	column_changed_handler_id = g_signal_connect(packetlist->view, "columns-changed", G_CALLBACK(column_dnd_changed_cb), NULL);
	g_object_set(packetlist->view, "has-tooltip", TRUE, NULL);
	g_signal_connect(packetlist->view, "query-tooltip",
			G_CALLBACK(query_packet_list_tooltip_cb), NULL);
	g_object_set_data(G_OBJECT(popup_menu_object), E_MPACKET_LIST_KEY, packetlist);

	/*		g_object_unref(packetlist); */ /* Destroy automatically with view for now */ /* XXX - Messes up freezing & thawing */

#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_override_font(packetlist->view, user_font_get_regular());
#else
	gtk_widget_modify_font(packetlist->view, user_font_get_regular());
#endif

	/* We need one extra column to store the entire PacketListRecord */
	for(i = 0; i < cfile.cinfo.num_cols; i++) {
		col_item = &cfile.cinfo.columns[i];
		renderer = gtk_cell_renderer_text_new();
		col = gtk_tree_view_column_new();
		gtk_tree_view_column_pack_start(col, renderer, TRUE);
		value = get_xalign_value(recent_get_column_xalign(i), right_justify_column(i, &cfile));
		g_object_set(G_OBJECT(renderer),
			     "xalign", value,
			     NULL);
		g_object_set(renderer,
			     "ypad", 0,
			     NULL);
		gtk_tree_view_column_add_attribute(col, renderer, "text", i);
		gtk_tree_view_column_set_cell_data_func(col, renderer,
							show_cell_data_func,
							GINT_TO_POINTER(i),
							NULL);

		escaped_title = ws_strdup_escape_char(col_item->col_title, '_');
		gtk_tree_view_column_set_title(col, escaped_title);
		g_free (escaped_title);
		gtk_tree_view_column_set_clickable(col, TRUE);
		gtk_tree_view_column_set_resizable(col, TRUE);
		gtk_tree_view_column_set_visible(col, get_column_visible(i));
		gtk_tree_view_column_set_sizing(col,GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_column_set_reorderable(col, TRUE); /* XXX - Should this be saved in the prefs? */

		g_object_set_data(G_OBJECT(col), E_MPACKET_LIST_COL_KEY, GINT_TO_POINTER(i));
		g_signal_connect(col, "clicked", G_CALLBACK(packet_list_column_clicked_cb), NULL);

		/*
		 * The column can't be adjusted to a size smaller than this
		 * XXX The minimum size will be the size of the title
		 * should that be limited for long titles?
		 */
		col_min_width = get_default_col_size (packetlist->view, cfile.cinfo.columns[i].col_title);
		if(col_min_width<COLUMN_WIDTH_MIN){
			gtk_tree_view_column_set_min_width(col, COLUMN_WIDTH_MIN);
		}else{
			gtk_tree_view_column_set_min_width(col, col_min_width);
		}

		/* Set the size the column will be displayed with */
		col_width = recent_get_column_width(i);
		if(col_width < 1) {
			gint fmt;
			const gchar *long_str;

			fmt = get_column_format(i);
			long_str = get_column_width_string(fmt, i);
			if(long_str){
				col_width = get_default_col_size (packetlist->view, long_str);
			}else{
				col_width = COLUMN_WIDTH_MIN;
			}
			gtk_tree_view_column_set_fixed_width(col, col_width);
		}else{
			gtk_tree_view_column_set_fixed_width(col, col_width);
		}

		gtk_tree_view_append_column(GTK_TREE_VIEW(packetlist->view), col);

		tooltip_text = get_column_tooltip(i);
		gtk_widget_set_tooltip_text(gtk_tree_view_column_get_button(col), tooltip_text);
		g_free(tooltip_text);
		g_signal_connect(gtk_tree_view_column_get_button(col), "button_press_event",
				 G_CALLBACK(packet_list_column_button_pressed_cb), col);

		if (i == 0) {  /* Default sort on first column */
			g_object_set_data(G_OBJECT(packetlist->view), E_MPACKET_LIST_COLUMN_KEY, col);
			g_object_set_data(G_OBJECT(packetlist->view), E_MPACKET_LIST_PREV_COLUMN_KEY, col);
		}
	}

	rebuild_visible_columns_menu ();

	return packetlist->view;
}

static frame_data *
packet_list_get_record(GtkTreeModel *model, GtkTreeIter *iter)
{
	frame_data *fdata;
	/* The last column is reserved for frame_data */
	gint record_column = gtk_tree_model_get_n_columns(model)-1;

	gtk_tree_model_get(model, iter,
			   record_column,
			   &fdata,
			   -1);

	return fdata;
}

void
packet_list_clear(void)
{
	packet_history_clear();

	packet_list_store_clear(packetlist);
	gtk_widget_queue_draw(packetlist->view);
	/* XXX is this correct in all cases?
	 * Reset the sort column, use packetlist as model in case the list is frozen.
	 */
	packet_list_sort_column(0, NULL, GTK_SORT_ASCENDING, FALSE);
}

void
packet_list_freeze(void)
{
	/* So we don't lose the model by the time we want to thaw it */
	g_object_ref(packetlist);

	/* Detach view from model */
	gtk_tree_view_set_model(GTK_TREE_VIEW(packetlist->view), NULL);
}

void
packet_list_thaw(void)
{
	/* Apply model */
	gtk_tree_view_set_model( GTK_TREE_VIEW(packetlist->view), GTK_TREE_MODEL(packetlist));

	/* Remove extra reference added by packet_list_freeze() */
	g_object_unref(packetlist);

	packets_bar_update();
}

void
packet_list_recreate_visible_rows(void)
{
	packet_list_recreate_visible_rows_list(packetlist);
}

void packet_list_resize_column(gint col)
{
	GtkTreeViewColumn *column;
	gint col_width;
	const gchar *long_str;

	long_str = packet_list_get_widest_column_string(packetlist, col);
	if(!long_str || strcmp("",long_str)==0)
		/* If we get an empty string leave the width unchanged */
		return;
	column = gtk_tree_view_get_column (GTK_TREE_VIEW(packetlist->view), col);
	col_width = get_default_col_size (packetlist->view, long_str);
	gtk_tree_view_column_set_fixed_width(column, col_width);
}

static void
packet_list_resize_columns(void)
{
	gint		progbar_loop_max;
	gint		progbar_loop_var;

	progbar_loop_max = cfile.cinfo.num_cols;

	for (progbar_loop_var = 0; progbar_loop_var < progbar_loop_max; ++progbar_loop_var)
		packet_list_resize_column(progbar_loop_var);
}

void
packet_list_resize_columns_cb(GtkWidget *widget _U_, gpointer data _U_)
{
	packet_list_resize_columns();
}

static void
scroll_to_current(void)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(top_level));

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(packetlist->view));
	/* model is filled with the current model as a convenience. */
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	scroll_to_and_select_iter(model, selection, &iter);

	/* Set the focus back where it was */
	if (focus)
		gtk_window_set_focus(GTK_WINDOW(top_level), focus);
}

void
packet_list_next(void)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(top_level));

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(packetlist->view));
	/* model is filled with the current model as a convenience. */
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	if (!gtk_tree_model_iter_next(model, &iter))
		return;

	scroll_to_and_select_iter(model, selection, &iter);

	/* Set the focus back where it was */
	if (focus)
		gtk_window_set_focus(GTK_WINDOW(top_level), focus);
}

void
packet_list_prev(void)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(top_level));

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(packetlist->view));
	/* model is filled with the current model as a convenience. */
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	path = gtk_tree_model_get_path(model, &iter);

	if (!gtk_tree_path_prev(path))
		return;

	if (!gtk_tree_model_get_iter(model, &iter, path))
		return;

	scroll_to_and_select_iter(model, selection, &iter);

	gtk_tree_path_free(path);

	/* Set the focus back where it was */
	if (focus)
		gtk_window_set_focus(GTK_WINDOW(top_level), focus);
}

static void
scroll_to_and_select_iter(GtkTreeModel *model, GtkTreeSelection *selection, GtkTreeIter *iter)
{
	GtkTreePath *path;

	g_assert(model);

	/* Select the row */
	if(!selection)
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(packetlist->view));
	gtk_tree_selection_select_iter (selection, iter);
	path = gtk_tree_model_get_path(model, iter);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(packetlist->view),
			path,
			NULL,
			TRUE,	/* use_align */
			0.5f,	/* row_align determines where the row is placed, 0.5 means center */
			0); 	/* The horizontal alignment of the column */

	/* "cursor-changed" signal triggers packet_list_select_cb() */
	/*  which will update the middle and bottom panes.              */
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(packetlist->view),
			path,
			NULL,
			FALSE); /* start_editing */

	gtk_tree_path_free(path);
}

void
packet_list_select_first_row(void)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(packetlist->view));
	GtkTreeIter iter;

	if(!gtk_tree_model_get_iter_first(model, &iter))
		return;

	scroll_to_and_select_iter(model, NULL, &iter);
	gtk_widget_grab_focus(packetlist->view);
}

void
packet_list_select_last_row(void)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(packetlist->view));
	GtkTreeIter iter;
	gint children;
	guint last_row;

	if((children = gtk_tree_model_iter_n_children(model, NULL)) == 0)
		return;

	last_row = children-1;
	if(!gtk_tree_model_iter_nth_child(model, &iter, NULL, last_row))
		return;

	scroll_to_and_select_iter(model, NULL, &iter);
}

void
packet_list_moveto_end(void)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(packetlist->view));
	GtkTreeIter iter;
	GtkTreePath *path;
	gint children;
	guint last_row;

	if((children = gtk_tree_model_iter_n_children(model, NULL)) == 0)
		return;

	last_row = children-1;
	if(!gtk_tree_model_iter_nth_child(model, &iter, NULL, last_row))
		return;

	path = gtk_tree_model_get_path(model, &iter);

	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(packetlist->view),
			path,
			NULL,
			TRUE,	/* use_align */
			0.5f,	/* row_align determines where the row is placed, 0.5 means center */
			0); 	/* The horizontal alignment of the column */

	gtk_tree_path_free(path);

}

gboolean
packet_list_check_end(void)
{
	gboolean at_end = FALSE;
	GtkAdjustment *adj;

#if GTK_CHECK_VERSION(3,0,0)
	adj = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (packetlist->view));
#else
	adj = gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(packetlist->view));
#endif
	g_return_val_if_fail(adj != NULL, FALSE);

	if (gtk_adjustment_get_value(adj) >= gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj)) {
		at_end = TRUE;
	}
#ifdef HAVE_LIBPCAP
	if (gtk_adjustment_get_value(adj) > 0 && at_end != last_at_end && at_end != auto_scroll_live) {
		main_auto_scroll_live_changed(at_end);
	}
#endif
	last_at_end = at_end;
	return at_end;
}

/*
 * Given a frame_data structure, scroll to and select the row in the
 * packet list corresponding to that frame.  If there is no such
 * row, return FALSE, otherwise return TRUE.
 */
gboolean
packet_list_select_row_from_data(frame_data *fdata_needle)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(packetlist->view));
	GtkTreeIter iter;

	/* Initializes iter with the first iterator in the tree (the one at the path "0")
	 * and returns TRUE. Returns FALSE if the tree is empty
	 */
	if(!gtk_tree_model_get_iter_first(model, &iter))
		return FALSE;

	do {
		frame_data *fdata;

		fdata = packet_list_get_record(model, &iter);

		if(fdata == fdata_needle) {
			scroll_to_and_select_iter(model, NULL, &iter);

			return TRUE;
		}
	} while (gtk_tree_model_iter_next(model, &iter));

	return FALSE;
}

void
packet_list_set_selected_row(gint row)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(packetlist->view));
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	GtkTreePath *path;

	path = gtk_tree_path_new_from_indices(row-1, -1);

	if (!gtk_tree_model_get_iter(model, &iter, path))
		return;

	/* Select the row */
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(packetlist->view));
	gtk_tree_selection_select_iter (selection, &iter);

	/* "cursor-changed" signal triggers packet_list_select_cb() */
	/*  which will update the middle and bottom panes.              */
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(packetlist->view),
			path,
			NULL,
			FALSE); /* start_editing */

	gtk_tree_path_free(path);
}

static gint
row_number_from_iter(GtkTreeIter *iter)
{
	gint row;
	gint *indices;
	GtkTreePath *path;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(packetlist->view));
	path = gtk_tree_model_get_path(model, iter);
	indices = gtk_tree_path_get_indices(path);
	g_assert(indices);
	/* Indices start from 0, but rows start from 1. Hence +1 */
	row = indices[0] + 1;

	gtk_tree_path_free(path);

	return row;
}

static void
packet_list_select_cb(GtkTreeView *tree_view, gpointer data _U_)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gint row;

	if ((selection = gtk_tree_view_get_selection(tree_view)) == NULL)
		return;

	if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
		return;

	row = row_number_from_iter(&iter);

	/* Check if already selected
	 */
	if (cfile.current_frame && cfile.current_row == row)
		return;

	/* Remove the hex display tab pages */
	while(gtk_notebook_get_nth_page(GTK_NOTEBOOK(byte_nb_ptr_gbl), 0))
		gtk_notebook_remove_page(GTK_NOTEBOOK(byte_nb_ptr_gbl), 0);

	cf_select_packet(&cfile, row);
	/* If searching the tree, set the focus there; otherwise, focus on the packet list */
	if (cfile.search_in_progress && cfile.decode_data) {
		gtk_widget_grab_focus(tree_view_gbl);
	} else {
		gtk_widget_grab_focus(packetlist->view);
	}

	/* Add newly selected frame to packet history (breadcrumbs) */
	packet_history_add(row);
}

static void
packet_list_double_click_cb(GtkTreeView *treeview, GtkTreePath *path _U_,
				GtkTreeViewColumn *col _U_, gpointer userdata _U_)
{
	new_packet_window(GTK_WIDGET(treeview), FALSE, FALSE);
}

gboolean
packet_list_get_event_row_column(GdkEventButton *event_button,
				     gint *physical_row, gint *row, gint *column)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(packetlist->view));
	GtkTreePath *path;
	GtkTreeViewColumn *view_column;

	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(packetlist->view),
					  (gint) event_button->x,
					  (gint) event_button->y,
					  &path, &view_column, NULL, NULL)) {
		GtkTreeIter iter;
		GList *cols;
		gint *indices;
		frame_data *fdata;

		/* Fetch indices */
		gtk_tree_model_get_iter(model, &iter, path);
		indices = gtk_tree_path_get_indices(path);
		g_assert(indices);
		/* Indices start from 0. Hence +1 */
		*row = indices[0] + 1;
		gtk_tree_path_free(path);

		/* Fetch physical row */
		fdata = packet_list_get_record(model, &iter);
		*physical_row = fdata->num;

		/* Fetch column */
		/* XXX -doesn't work if columns are re-arranged? */
		cols = gtk_tree_view_get_columns(GTK_TREE_VIEW(packetlist->view));
		*column = g_list_index(cols, (gpointer) view_column);
		g_list_free(cols);

		return TRUE;
	}
	else
		return FALSE;
}

frame_data *
packet_list_get_row_data(gint row)
{
	GtkTreePath *path = gtk_tree_path_new();
	GtkTreeIter iter;
	frame_data *fdata;

	g_assert(row > 0);
	gtk_tree_path_append_index(path, row-1);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(packetlist), &iter, path);

	fdata = packet_list_get_record(GTK_TREE_MODEL(packetlist), &iter);

	gtk_tree_path_free(path);

	return fdata;
}

static void
show_cell_data_func(GtkTreeViewColumn *col _U_, GtkCellRenderer *renderer,
			GtkTreeModel *model, GtkTreeIter *iter, gpointer data _U_)
{
	frame_data *fdata = packet_list_get_record(model, iter);

	gboolean color_on;
	GdkColor fg_gdk;
	GdkColor bg_gdk;

	if (fdata->flags.ignored) {
		color_t_to_gdkcolor(&fg_gdk, &prefs.gui_ignored_fg);
		color_t_to_gdkcolor(&bg_gdk, &prefs.gui_ignored_bg);
		color_on = TRUE;
	} else if (fdata->flags.marked) {
		color_t_to_gdkcolor(&fg_gdk, &prefs.gui_marked_fg);
		color_t_to_gdkcolor(&bg_gdk, &prefs.gui_marked_bg);
		color_on = TRUE;
	} else if (fdata->color_filter) {
		const color_filter_t *color_filter = (const color_filter_t *)fdata->color_filter;

		color_t_to_gdkcolor(&fg_gdk, &color_filter->fg_color);
		color_t_to_gdkcolor(&bg_gdk, &color_filter->bg_color);
		color_on = enable_color;
	} else
		color_on = FALSE;

	if (color_on) {
		g_object_set(renderer,
			 "foreground-gdk", &fg_gdk,
			 "foreground-set", TRUE,
			 "background-gdk", &bg_gdk,
			 "background-set", TRUE,
			 NULL);
	} else {
		g_object_set(renderer,
			 "foreground-set", FALSE,
			 "background-set", FALSE,
			 NULL);
	}
}

void
packet_list_enable_color(gboolean enable)
{
	enable_color = enable;
	gtk_widget_queue_draw (packetlist->view);
}

/* Redraw the packet list *and* currently-selected detail */
void
packet_list_queue_draw(void)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gint row;

	gtk_widget_queue_draw (packetlist->view);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(packetlist->view));
	if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
		return;
	row = row_number_from_iter(&iter);
	cf_select_packet(&cfile, row);
}

void
packet_list_set_font(PangoFontDescription *font)
{
#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_override_font(packetlist->view, font);
#else
	gtk_widget_modify_font(packetlist->view, font);
#endif
}


/* call this after last set_frame_mark is done */
static void
mark_frames_ready(void)
{
	packets_bar_update();
	packet_list_queue_draw();
}

static void
set_frame_mark(gboolean set, frame_data *fdata)
{
	if (set)
		cf_mark_frame(&cfile, fdata);
	else
		cf_unmark_frame(&cfile, fdata);
}

void
packet_list_mark_frame_cb(GtkWidget *w _U_, gpointer data _U_)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	frame_data *fdata;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(packetlist->view));
	/* model is filled with the current model as a convenience. */
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	fdata = packet_list_get_record(model, &iter);

	set_frame_mark(!fdata->flags.marked, fdata);
	mark_frames_ready();
}

static void
mark_all_displayed_frames(gboolean set)
{
	/* XXX: we might need a progressbar here */
	guint32 framenum;
	frame_data *fdata;
	for (framenum = 1; framenum <= cfile.count; framenum++) {
		fdata = frame_data_sequence_find(cfile.frames, framenum);
		if( fdata->flags.passed_dfilter )
			set_frame_mark(set, fdata);
	}
}

void
packet_list_mark_all_displayed_frames_cb(GtkWidget *w _U_, gpointer data _U_)
{
	mark_all_displayed_frames(TRUE);
	mark_frames_ready();
}

void
packet_list_unmark_all_displayed_frames_cb(GtkWidget *w _U_, gpointer data _U_)
{
	mark_all_displayed_frames(FALSE);
	mark_frames_ready();
}

static void
toggle_mark_all_displayed_frames(void)
{
	/* XXX: we might need a progressbar here */
	guint32 framenum;
	frame_data *fdata;
	for (framenum = 1; framenum <= cfile.count; framenum++) {
		fdata = frame_data_sequence_find(cfile.frames, framenum);
		if( fdata->flags.passed_dfilter )
			set_frame_mark(!fdata->flags.marked, fdata);
	}
}

void
packet_list_toggle_mark_all_displayed_frames_cb(GtkWidget *w _U_, gpointer data _U_)
{
	toggle_mark_all_displayed_frames();
	mark_frames_ready();
}


static void
set_frame_ignore(gboolean set, frame_data *fdata)
{
	if (set)
		cf_ignore_frame(&cfile, fdata);
	else
		cf_unignore_frame(&cfile, fdata);
}

void
packet_list_ignore_frame_cb(GtkWidget *w _U_, gpointer data _U_)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	frame_data *fdata;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(packetlist->view));
	/* model is filled with the current model as a convenience. */
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	fdata = packet_list_get_record(model, &iter);
	set_frame_ignore(!fdata->flags.ignored, fdata);
	redissect_packets();
}

static void
ignore_all_displayed_frames(gboolean set)
{
	guint32 framenum;
	frame_data *fdata;

	/* XXX: we might need a progressbar here */
	for (framenum = 1; framenum <= cfile.count; framenum++) {
		fdata = frame_data_sequence_find(cfile.frames, framenum);
		if( fdata->flags.passed_dfilter )
			set_frame_ignore(set, fdata);
	}
	redissect_packets();
}

void
packet_list_ignore_all_displayed_frames_cb(GtkWidget *w _U_, gpointer data _U_)
{
	if(cfile.displayed_count < cfile.count){
		frame_data *fdata;
		/* Due to performance impact with large captures, don't check the filtered list for
		an ignored frame; just check the first. If a ignored frame exists but isn't first and
		the user wants to unignore all the displayed frames, they will just re-exec the shortcut. */
		fdata = frame_data_sequence_find(cfile.frames, cfile.first_displayed);
		if (fdata->flags.ignored==TRUE) {
			ignore_all_displayed_frames(FALSE);
		} else {
			ignore_all_displayed_frames(TRUE);
		}
	}
}

static void
unignore_all_frames(void)
{
	guint32 framenum;
	frame_data *fdata;

	/* XXX: we might need a progressbar here */
	for (framenum = 1; framenum <= cfile.count; framenum++) {
		fdata = frame_data_sequence_find(cfile.frames, framenum);
		set_frame_ignore(FALSE, fdata);
	}
	redissect_packets();
}

void
packet_list_unignore_all_frames_cb(GtkWidget *w _U_, gpointer data _U_)
{
	unignore_all_frames();
}


static void
untime_reference_all_frames(void)
{
	/* XXX: we might need a progressbar here */
	guint32 framenum;
	frame_data *fdata;
	for (framenum = 1; framenum <= cfile.count && cfile.ref_time_count > 0; framenum++) {
		fdata = frame_data_sequence_find(cfile.frames, framenum);
		if (fdata->flags.ref_time == 1) {
			set_frame_reftime(FALSE, fdata, cfile.current_row);
		}
	}
}

void
packet_list_untime_reference_all_frames_cb(GtkWidget *w _U_, gpointer data _U_)
{
	untime_reference_all_frames();
}


guint
packet_list_get_column_id (gint col_num)
{
	GtkTreeViewColumn *column = gtk_tree_view_get_column (GTK_TREE_VIEW(packetlist->view), col_num);
	gint col_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(column), E_MPACKET_LIST_COL_KEY));

	return col_id;
}

void
packet_list_copy_summary_cb(gpointer data _U_, copy_summary_type copy_type)
{
	gint col;
	gchar *celltext;
	GString* text;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gboolean first_col = TRUE;

	if(CS_CSV == copy_type) {
		text = g_string_new("\"");
	} else {
		text = g_string_new("");
	}

	if (cfile.current_frame) {
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(packetlist->view));
		/* model is filled with the current model as a convenience.  */
		if (!gtk_tree_selection_get_selected(selection, &model, &iter))
			return;

		for(col = 0; col < cfile.cinfo.num_cols; ++col) {
			if (get_column_visible(col)) {
				if(!first_col) {
					if(CS_CSV == copy_type) {
						g_string_append(text,"\",\"");
					} else {
						g_string_append_c(text, '\t');
					}
				}

				gtk_tree_model_get(model, &iter, packet_list_get_column_id(col), &celltext, -1);
				g_string_append(text,celltext);
				g_free(celltext);
				first_col = FALSE;
			}
		}
		if(CS_CSV == copy_type) {
			g_string_append_c(text,'"');
		}
		copy_to_clipboard(text);
	}
	g_string_free(text,TRUE);
}

gchar *
packet_list_get_packet_comment(void)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	frame_data *fdata;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(packetlist->view));
	/* model is filled with the current model as a convenience. */
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return NULL;

	fdata = packet_list_get_record(model, &iter);

	return cf_get_comment(&cfile, fdata);
}

void
packet_list_return_all_comments(GtkTextBuffer *buffer)
{
	guint32 framenum;
	frame_data *fdata;
	gchar *buf_str;

	for (framenum = 1; framenum <= cfile.count ; framenum++) {
		char *pkt_comment;

		fdata = frame_data_sequence_find(cfile.frames, framenum);
		pkt_comment = cf_get_comment(&cfile, fdata);
		if (pkt_comment) {
			buf_str = g_strdup_printf("Frame %u: %s \n\n",framenum, pkt_comment);
			gtk_text_buffer_insert_at_cursor (buffer, buf_str, -1);
			g_free(buf_str);
			g_free(pkt_comment);
		}
		if (gtk_text_buffer_get_char_count(buffer) > MAX_COMMENTS_TO_FETCH) {
			buf_str = g_strdup_printf("[ Comment text exceeds %s. Stopping. ]",
						  format_size(MAX_COMMENTS_TO_FETCH, (format_size_flags_e)(format_size_unit_bytes|format_size_prefix_si)));
			gtk_text_buffer_insert_at_cursor (buffer, buf_str, -1);
			return;
		}
	}
}

void
packet_list_update_packet_comment(gchar *new_packet_comment)
{

	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	frame_data *fdata;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(packetlist->view));
	/* model is filled with the current model as a convenience. */
	if (!gtk_tree_selection_get_selected(selection, &model, &iter))
		return;

	fdata = packet_list_get_record(model, &iter);

	/* Check if we are clearing the comment */
	if(strlen(new_packet_comment) == 0) {
		g_free(new_packet_comment);
		new_packet_comment = NULL;
	}

	cf_set_user_packet_comment(&cfile, fdata, new_packet_comment);

	g_free(new_packet_comment);

	/* Update the main window, as we now have unsaved changes. */
	main_update_for_unsaved_changes(&cfile);

	packet_list_queue_draw();

}

void
packet_list_recent_write_all(FILE *rf)
{
	gint col, width, num_cols, col_fmt;
	GtkTreeViewColumn *tree_column;
	gchar xalign;

	fprintf (rf, "%s:", RECENT_KEY_COL_WIDTH);
	num_cols = g_list_length(prefs.col_list);
	for (col = 0; col < num_cols; col++) {
		col_fmt = get_column_format(col);
		if (col_fmt == COL_CUSTOM) {
			fprintf (rf, " %%Cus:%s,", get_column_custom_fields(col));
		} else {
			fprintf (rf, " %s,", col_format_to_string(col_fmt));
		}
		tree_column = gtk_tree_view_get_column(GTK_TREE_VIEW(packetlist->view), col);
		width = gtk_tree_view_column_get_width(tree_column);
		xalign = recent_get_column_xalign (col);
		if (width == 0) {
			/* We have not initialized the packet list yet, use old values */
			width = recent_get_column_width (col);
		}
		fprintf (rf, " %d", width);
		if (xalign != COLUMN_XALIGN_DEFAULT) {
			fprintf (rf, ":%c", xalign);
		}
		if (col != num_cols-1) {
			fprintf (rf, ",");
		}
	}
	fprintf (rf, "\n");
}

GtkWidget *
packet_list_get_widget(void)
{
	g_assert(packetlist);
	g_assert(packetlist->view);
	return packetlist->view;
}

void
packet_list_colorize_packets(void)
{
	packet_list_reset_colorized(packetlist);
	gtk_widget_queue_draw (packetlist->view);
}

static gboolean
query_packet_list_tooltip_cb(GtkWidget *widget, gint x, gint y, gboolean keyboard_tip, GtkTooltip *tooltip, gpointer data _U_)
{
	GtkTreeIter iter;
	GtkTreeView *tree_view = GTK_TREE_VIEW(widget);
	GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
	GtkTreePath *path = NULL;
	GtkTreeViewColumn *column;
	gint col, num_cols;
	frame_data *fdata;
	GtkCellRenderer* renderer=NULL;
	GList *renderer_list;
	gboolean result = FALSE;

	if (!gtk_tree_view_get_tooltip_context(tree_view, &x, &y, keyboard_tip, &model, &path, &iter))
		return result;

	if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(tree_view), x, y, NULL, &column, NULL, NULL)) {
		char *pkt_comment;

		num_cols = g_list_length(prefs.col_list);

		for (col = 0; col < num_cols; col++) {
			if (gtk_tree_view_get_column(tree_view, col) == column)
				break;
		}

		fdata = packet_list_get_record(model, &iter);
		pkt_comment = cf_get_comment(&cfile, fdata);

		if (pkt_comment != NULL) {
			gtk_tooltip_set_markup(tooltip, pkt_comment);
			renderer_list = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(column));
			/* get the first renderer */
			if (g_list_first(renderer_list)) {
				renderer = (GtkCellRenderer*)g_list_nth_data(renderer_list, 0);
				gtk_tree_view_set_tooltip_cell (tree_view, tooltip, path, column, renderer);
			}
			g_list_free(renderer_list);
			g_free(pkt_comment);
			result = TRUE;
		}
	}
	gtk_tree_path_free(path);

	return result;
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
