/* gui_stat_util.h
 * gui functions used by stats
 * Copyright 2003 Lars Roland
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

#ifndef __GTK_STAT_UTIL_H__
#define __GTK_STAT_UTIL_H__

#include <gtk/gtk.h>


/** @file
 *  Utilities for statistics.
 */

/** Columns definition
 */
typedef struct {
    GType      type;   /* column type */
    gint       align;  /* alignement  */
    const char *title; /* column title */
} stat_column;

/** Init a window for stats, set title and display used filter in window.
 *
 * @param window the window
 * @param mainbox the vbox for the window
 * @param title the title for the window
 * @param filter the filter string
 */
extern void init_main_stat_window(GtkWidget *window, GtkWidget *mainbox, const char *title, const char *filter);

/** Create a stats table, using a scrollable gtkclist.
 *
 * @param scrolled_window the scrolled window
 * @param vbox the vbox for the window
 * @param columns number of columns
 * @param headers columns title and type, G_TYPE_POINTER is illegal, there's no default cell renderer for it
 */
extern GtkTreeView *create_stat_table(GtkWidget *scrolled_window, GtkWidget *vbox, int columns, const stat_column *headers);

#endif /* __GUI_STAT_UTIL_H__ */
