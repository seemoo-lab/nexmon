/* dfilter-macro.h
 *
 * $Id: dfilter-macro.h 22427 2007-07-30 23:32:47Z lego $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _DFILTER_MACRO_H
#define _DFILTER_MACRO_H

#define DFILTER_MACRO_FILENAME "dfilter_macros"


typedef struct _dfilter_macro_t {
	gchar* name; /* the macro id */
	gchar* text; /* raw data from file */
	gboolean usable; /* macro is usable */
	gchar** parts; /* various segments of text between insertion targets */
	int* args_pos; /* what's to be inserted */
	int argc; /* the expected number of arguments */
	void* priv; /* a copy of text that contains every c-string in parts */
} dfilter_macro_t;

/* loop over the macros list */
typedef void (*dfilter_macro_cb_t)(dfilter_macro_t*, void*);
void dfilter_macro_foreach(dfilter_macro_cb_t, void*);

/* save dfilter macros to a file */
void dfilter_macro_save(const gchar*, gchar**);

/* dumps the macros in the list (debug info, not formated as in the macros file) */
void dfilter_macro_dump(void);

/* applies all macros to the given text and returns the resulting string or NULL on failure */
gchar* dfilter_macro_apply(const gchar* text, guint depth, const gchar** error);

void dfilter_macro_init(void);

void dfilter_macro_get_uat(void**);

void dfilter_macro_build_ftv_cache(void* tree_root);

#endif /* _DFILTER_MACRO_H */
