/* tap_param_dlg.h
 * Header file for parameter dialog used by gui taps
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

#ifndef __TAP_PARAM_DLG_H__
#define __TAP_PARAM_DLG_H__

/*
 * You can easily add a parameter dialog for your gui tap by using
 * the following infrastructure:
 *
 * Define a global structure of tap_param_dlg within your stat source file.
 * Initiate it with:
 * 1) a title string for the Dialog Window
 * 2) the init string, which is the same as the string after "-z" option without
 *    the filter string and without the separating comma.
 * 3) a pointer to the init function of the tap, which will be called when you click
 *    on the start button in the display filter dialog.
 * 4) the index with "-1"
 *
 * Within register_tap_menu_yourtap(void), call register_param_stat()
 * with a pointer to the tap_param_dlg structure, a string for the
 * menu item (don't put "..." at the end, register_param_stat() will
 * add it for you), and the REGISTER_STAT_GROUP_ value for the stat
 * group to which your stat should belong.
 *
 * Usage:
 *
 * tap_param_dlg my_tap_param_dlg = {
 *	"My Title",
 *	"myproto,mytap",
 *	gtk_mytap_init,
 *	-1
 * };
 *
 * register_tap_menu_mytap(void) {
 *   register_param_stat(&my_tap_param_dlg, "My Menu Item",
 *       REGISTER_STAT_GROUP_my_group);
 * }
 *
 * See also: h225_ras_srt.c
 *
 */

#include <epan/params.h>
#include <epan/stat_tap_ui.h>

typedef struct _tap_param_dlg {
	const char *win_title;		/* title */
	const char *init_string;	/* the string to call the tap without a filter via "-z" option */
	void (* tap_init_cb)(const char *,void*);	/* callback to init function of the tap */
	gint index;			/* initiate this value always with "-1" */
	size_t nparams;			/* number of parameters */
	tap_param *params;		/* pointer to table of parameter info */
	gpointer user_data;		/* Optional "dialog specific" data */
} tap_param_dlg;

/*
 * Register a stat that has a parameter dialog.
 * We register it both as a command-line stat and a menu item stat.
 */
void register_param_stat(tap_param_dlg *info, const char *name,
    register_stat_group_t group);

void tap_param_dlg_cb(GtkAction *action, gpointer user_data);

/* This will update the titles of the dialog windows when we load a new capture file. */
void tap_param_dlg_update (void);

#endif /* __TAP_PARAM_DLG_H__ */
