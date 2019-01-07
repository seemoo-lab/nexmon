/* pixmap_save.h
 * Routines for saving pixmaps using the Gdk-Pixmap library
 * Copyright 2007, Stephen Fisher (see AUTHORS file)
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

#ifndef __PIXMAP_SAVE_H__
#define __PIXMAP_SAVE_H__

/* Callback to be tied to a save button.  This function will pop-up a dialog
 * asking for options to save the graph with (such as file type). */
void pixmap_save_cb(GtkWidget *w, gpointer pixmap_ptr);

typedef struct _surface_info_t {
	cairo_surface_t *surface;
	gint	width;
	gint	height;
} surface_info_t;
#endif /* __PIXMAP_SAVE_H__ */
